# Migrating to la_avdecc 5.x

Version 5.0 introduces native **redundancy** support in the controller library: a controller can now be created with two physical network interfaces (a *Primary* and a *Secondary*), matching the Milan redundant network topology. The controller transparently merges the entity discovery from both interfaces, routes commands to the most appropriate interface (with automatic failover), and maintains one unsolicited-notifications subscription per interface so that the loss of one network does not desynchronize the entity model.

This is an API-breaking release (`InterfaceVersion` 500 for both `la_avdecc` and `la_avdecc_controller`), but the migration effort depends on whether you use the redundant mode:

- **If you keep using a single interface**: the behavior is strictly identical to 4.x. The only required change is mechanical: a few `Controller::Observer` callbacks gained a trailing `Controller::InterfaceType` parameter, which is always `InterfaceType::Primary` in single-interface mode and can simply be ignored.
- **If you opt in to the redundant mode**: read the paradigm section below, as your observers become responsible for aggregating per-interface states.

## The redundant controller paradigm

In redundant mode the controller drives two physical interfaces, identified by `Controller::InterfaceType::Primary` and `Controller::InterfaceType::Secondary` (the order of the configurations passed to `create()`). Each interface hosts its own controller entity (distinct EntityID, see `getControllerEID(InterfaceType)`) and is therefore, from the entities' point of view, an independent controller: an independent transport, and an independent unsolicited-notifications subscriber.

A redundant entity remains fully usable as long as **at least one** interface can reach it: discovery information from both interfaces is merged into a single `ControlledEntity`, commands are automatically routed (and retried on the other interface for transport-class failures), and the model keeps receiving updates through whichever interface still holds a valid unsolicited subscription.

Consequently, events that report an interface-related condition are now emitted **once per interface**, carrying the `InterfaceType` they apply to. **It is the observer's responsibility to track the per-interface states and decide when a global condition is reached** (every interface down, no interface subscribed anymore, ...). This is what prevents an application from displaying "network lost" or "unsolicited notifications lost" when only one of the two networks is affected while the controller is actually still fully operational through the other one.

## Observer callbacks changes

### `onTransportError`

```cpp
// 4.x
void onTransportError(Controller const* controller);
// 5.x
void onTransportError(Controller const* controller, Controller::InterfaceType interfaceType);
```

Fired once per interface encountering a fatal transport error. In single-interface mode, `interfaceType` is always `Primary` and the meaning is unchanged. In redundant mode, receiving it for one interface means the controller now operates on the other one alone; the controller should be considered dead only once the event has been received for every interface it was created with.

### `onUnsolicitedRegistrationChanged`

```cpp
// 4.x
void onUnsolicitedRegistrationChanged(Controller const* controller, ControlledEntity const* entity, bool isSubscribed, bool triggeredByEntity);
// 5.x
void onUnsolicitedRegistrationChanged(Controller const* controller, ControlledEntity const* entity, bool isSubscribed, bool triggeredByEntity, Controller::InterfaceType interfaceType);
```

Fired for each interface whose unsolicited-notifications subscription state changes. In single-interface mode, `interfaceType` is always `Primary` and the meaning is unchanged. In redundant mode, each interface is a separate subscriber on the entity side: losing the subscription on one interface (transient link loss, sequenceID gap, entity-initiated deregistration, ...) is reported for that interface while the model stays in sync through the other one (the library automatically re-registers the affected interface when possible). The entity model should be considered out-of-sync only when **no** interface is subscribed anymore, at which point a user-decided `refreshEntity()` is the way to resynchronize (the library never re-enumerates on its own).

### Statistics events

The following callbacks gained a trailing `Controller::InterfaceType interfaceType` parameter indicating on which interface the event occurred. The counter values remain aggregated per entity across interfaces; in single-interface mode `interfaceType` is always `Primary`.

- `onAecpRetryCounterChanged`
- `onAecpTimeoutCounterChanged`
- `onAecpUnexpectedResponseCounterChanged`
- `onAecpResponseAverageTimeChanged`
- `onAemAecpUnsolicitedCounterChanged`
- `onAemAecpUnsolicitedLossCounterChanged`
- `onMvuAecpUnsolicitedCounterChanged`
- `onMvuAecpUnsolicitedLossCounterChanged`

## New API for the redundant mode

- `Controller::InterfaceType` enum (`Primary` / `Secondary`), `Controller::NumInterfaces` and `Controller::AllInterfaceTypes`.
- `Controller::InterfaceConfiguration` struct (`protocolInterfaceType`, `networkInterfaceID`, optional `executorName`).
- `Controller::create(std::vector<InterfaceConfiguration> const&, std::uint16_t progID, UniqueIdentifier entityModelID, std::string const& preferedLocale, entity::model::EntityTree const*, entity::controller::Interface const* virtualEntityInterface)` — pass 2 configurations for a redundant controller, or 1 to get the exact single-interface behavior. Both configurations must currently share the same executor. The two `networkInterfaceID` values must differ.
- `Controller::getControllerEID(InterfaceType = InterfaceType::Primary)` — per-interface controller EntityID.
- `Controller::Error::InvalidInterfaceConfiguration` — raised by `create()` when the configuration list is invalid (empty, more than 2 entries, or duplicate interfaces).
- The single-interface `Controller::create()` overload from 4.x is unchanged.

## Migration checklist

1. Recompile: the compiler will point at every overridden observer callback whose signature changed; add the trailing `Controller::InterfaceType` parameter (ignore it if you do not use the redundant mode).
2. C# / SWIG bindings follow the same signatures: add the `Controller.InterfaceType` parameter to the corresponding overrides.
3. If (and only if) you adopt the redundant mode: create the controller with 2 `InterfaceConfiguration` entries, and make your observers track per-interface states as described above.
