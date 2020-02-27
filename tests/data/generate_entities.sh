#!/usr/bin/env bash

function generate_entity()
{
	local index="$1"
	local entityID
	local primaryMac
	local secondaryMac
	local entityName
	local serialNumber
	local fileName
	local outputFolder="generatedEntities"
	
	mkdir -p $outputFolder
	
	local byteResult=$(($index / 256))
	local byteModulo=$(($index % 256))
	
	printf -v entityID "0x001B92FFFF00%.4X" $index
	printf -v primaryMac "00:1B:92:00:%.2X:%.2X" $byteResult $byteModulo
	printf -v secondaryMac "00:1B:92:01:%.2X:%.2X" $byteResult $byteModulo
	printf -v entityName "Virtual Entity %.4X" $index
	printf -v serialNumber "00%.4X" $index
	printf -v fileName "$outputFolder/Entity_%.4X.json" $index
	
	echo "Generating Entity '$entityName' (eid=$entityID mac1=$primaryMac mac2=$secondaryMac s/n=$serialNumber)"
	
	sed -e "s/###ENTITY_ID###/$entityID/" "TalkerListener_template.json" > "$fileName"
	sed -i -e "s/###PRIMARY_MAC_ADDRESS###/$primaryMac/" "$fileName"
	sed -i -e "s/###SECONDARY_MAC_ADDRESS###/$secondaryMac/" "$fileName"
	sed -i -e "s/###ENTITY_NAME###/$entityName/" "$fileName"
	sed -i -e "s/###SERIAL_NUMBER###/$serialNumber/" "$fileName"
}

for i in $(seq 0 500)
do
	generate_entity $i
done
