### How to update the AEM schemas

The algorithm used by the library to generate dump files that comply with the AEM schemas can be found in the following source files:
- For the node hierarchy, see the `src/entity/entityModelJsonSerializer.cpp` file.
- For the static descriptors properties, see the `include/la/avdecc/internals/jsonTypes.hpp` file.
- For more details about the static descriptors themselves, see the `include/la/avdecc/internals/entityModelTreeStatic.hpp` file.

When a new `dump version` is released, a new AEM schema name `schema.json` should be created and stored in a subfolder of this directory which is named after the `dump version`. The `dump_version` field should strictly check for the exact version of the dump file. The `dump_version` is controlled by the `ControlledEntity_DumpVersion` variable from `src/controller/avdeccControllerJsonTypes.hpp`.

When a minor revision of a schema needs to be made, simply update the `schema_version` field in the affected `schema.json` file.
