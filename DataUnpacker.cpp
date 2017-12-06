#include "DataUnpacker.hpp"
#include "DataReader.hpp"

DataUnpacker::DataUnpacker(){};

ADCDataItem::ADCDataItem(std::pair < unsigned int, unsigned int> inData){

	dataType = 3;
	unsigned int chIdentity = (inData.first >> 16) & 0xFFF; //Word 0, bits 16:27
	fee64ID = (chIdentity >> 6) & 0x003F;  					//Top 6 bit of chIdentity (22:27) are FEE64 number
	channelID = chIdentity & 0x003f;						//Bottom 6 bits of chIdentity (16:21) are channel number
	adcRange = (inData.first >> 28 ) & 0x0001;				//Bit 28 - Veto bit used as ADC range
	adcData = (inData.first & 0xFFFF); 						//Bits 0:15 of Word 0 is ADC data
	timestampLSB = (inData.second & 0x0FFFFFFF);				//Word 1, bits 0:27 - Timestamp LSB

}
void ADCDataItem::SetTimestamp(unsigned long MSB){
	timestamp = (MSB << 28) | timestampLSB;
}

InformationDataItem::InformationDataItem(std::pair < unsigned int, unsigned int> inData){

	dataType = 2;

	infoField = ( inData.first & 0x000FFFFF); 				//Word 0, bits 0:19 is the information field
	infoCode = (inData.first >> 20) & 0x000F;				//Word 0, bits 20:23 - Information code
	fee64ID = (inData.first >> 24) & 0x003F;				//Word 0, bits 24:29 - FEE64 module number
	timestampLSB = (inData.second & 0x0FFFFFFF);			//Word 1, bits 0:27 - Timestamp LSB

	if ( infoCode == 2 || infoCode == 3 || infoCode == 4){
		timestampMSB = infoField;
		timestamp = (timestampMSB << 28) | timestampLSB;
	}

	if ( infoCode == 8){

		corrScalerIndex = (infoField & 0x000F0000) >> 16;	//Index of correlation scaler
		corrScalerTimestamp = (infoField & 0x0000FFFF);		//Bits with timestamp

	}


}
void InformationDataItem::SetTimestamp(unsigned long MSB){
	timestamp = ((MSB <<28 ) | timestampLSB);
}
unsigned long InformationDataItem::GetTimestampMSB(){
	return timestampMSB;
}
unsigned int InformationDataItem::GetInfoCode(){
	return infoCode;
}

unsigned int InformationDataItem::GetCorrScalerIndex(){
	return corrScalerIndex;
}
unsigned long InformationDataItem::GetCorrScalerTimestamp(){
	return corrScalerTimestamp;
}
unsigned int InformationDataItem::GetFEE64ID(){
	return fee64ID;
}
unsigned long InformationDataItem::GetTimestamp(){
	return timestamp;
}
unsigned long InformationDataItem::GetTimestampLSB(){
	return timestampLSB;
}
void DataUnpacker::InitialiseDataUnpacker(){
	for (int i = 0; i < 24; i++){
		pauseItemCounter[i] = 0;
		resumeItemCounter[i] = 0;
		sync100Counter[i] = 0;
		correlationScalerData0[i] = 0;
		correlationScalerData1[i] = 0;
	}
}
void DataUnpacker::BeginDataUnpacker(DataReader & dataReader){

	for(;;){
	dataWords = dataReader.ReadFromBuffer();
	//std::cout << "first word " << dataWords.first << " second word " << dataWords.second << std::endl;

	UnpackWords(dataWords);

	}
}

void DataUnpacker::UnpackWords(std::pair < unsigned int, unsigned int> wordsIn){
	//Takes the data Words that have been read in and determines the data type and
	//unpacks the data accordingly.

	dataType = ( (wordsIn.first >>30) & 0x3);
	if (dataType ==3 ){
		//ADC data item - Unpack into ADCDataItem format
		ADCDataItem adcDataItem(wordsIn);

		if (timestampMSBStatus){
			adcDataItem.SetTimestamp(timestampMSB);
		}

		
	}
	else if (dataType == 2){

		InformationDataItem informationDataItem(wordsIn);

		if(informationDataItem.GetInfoCode() == 2){
			timestampMSB = informationDataItem.GetTimestampMSB();
			timestampMSBStatus = true;
			pauseItemCounter[informationDataItem.GetFEE64ID()-1] += 1;

			#ifdef DEB_UNPACKER
				std::cout << "\nTimestamp MSB Updated - " << timestampMSB << " PAUSE information item\n" << std::endl;
			#endif
		}
		else if(informationDataItem.GetInfoCode() == 3){
			timestampMSB = informationDataItem.GetTimestampMSB();
			timestampMSBStatus = true;
			resumeItemCounter[informationDataItem.GetFEE64ID()-1] += 1;

			#ifdef DEB_UNPACKER
				std::cout << "\nTimestamp MSB Updated - " << timestampMSB << " RESUME information item\n" << std::endl;
			#endif
		}
		else if(informationDataItem.GetInfoCode() == 4){
			timestampMSB = informationDataItem.GetTimestampMSB();
			timestampMSBStatus = true;
			sync100Counter[informationDataItem.GetFEE64ID()-1] += 1;

			#ifdef DEB_UNPACKER
				std::cout << "\nTimestamp MSB Updated - " << timestampMSB << " SYNC100 information item\n" << std::endl;
			#endif
		}
		else if(informationDataItem.GetInfoCode() == 8){

			if(timestampMSBStatus){//No MSB information in scaler so can't set timestamp in constructor
				informationDataItem.SetTimestamp(timestampMSB);

			if(informationDataItem.GetCorrScalerIndex() == 0){
				correlationScalerData0[informationDataItem.GetFEE64ID()] = informationDataItem.GetCorrScalerTimestamp();
			}
			else if(informationDataItem.GetCorrScalerIndex() == 1){
				correlationScalerData1[informationDataItem.GetFEE64ID()] = informationDataItem.GetCorrScalerTimestamp();
			}
			else if(informationDataItem.GetCorrScalerIndex() == 2){
				correlationScaler = (informationDataItem.GetCorrScalerTimestamp() << 32) | (correlationScalerData1[informationDataItem.GetFEE64ID()] << 16)
									| (correlationScalerData0[informationDataItem.GetFEE64ID()]);

			correlationScalerOffset = (4*correlationScaler)- informationDataItem.GetTimestamp();

				#ifdef DEB_CORRELATION
					std::cout << "Correlation scaler " << correlationScaler << " Offset = " << correlationScalerOffset << std::endl;
				#endif
			}
			} 

		}

	}
}
