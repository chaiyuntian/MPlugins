// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.

#include "SocketClientPluginTCPClient.h"


USocketClientPluginTCPClient::USocketClientPluginTCPClient(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	//This prevents the garbage collector from killingand deleting the class from RAM.
	this->AddToRoot();
}


void USocketClientPluginTCPClient::socketClientTCPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID){}
void USocketClientPluginTCPClient::receiveTCPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const FString clientConnectionID){}

void USocketClientPluginTCPClient::connect(USocketClientBPLibrary* mainLibP, FString domainOrIPP, int32 portP, EReceiveFilterClient receiveFilter, FString connectionIDP){
	mainLib = mainLibP;
	connectionID = connectionIDP;
	domainOrIP = domainOrIPP;
	port = portP;
	tcpConnectionThread = new FServerConnectionThread(mainLib, connectionID, receiveFilter, domainOrIP, port,this);
}

void USocketClientPluginTCPClient::sendMessage(FString message, TArray<uint8> byteArray){
	if (run && tcpSendThread != nullptr) {
		tcpSendThread->sendMessage(message, byteArray);
	}
}

void USocketClientPluginTCPClient::closeConnection(){
	setRun(false);
	tcpConnectionThread = nullptr;
	tcpSendThread = nullptr;
	mainLib = nullptr;
}

bool USocketClientPluginTCPClient::isRun(){
	return run;
}

void USocketClientPluginTCPClient::setRun(bool runP) {
	run = runP;
}

FString USocketClientPluginTCPClient::getConnectionID(){
	return connectionID;
}

void USocketClientPluginTCPClient::setSocket(FSocket* socketP){
	socket = socketP;
}

FSocket* USocketClientPluginTCPClient::getSocket(){
	return socket;
}

void USocketClientPluginTCPClient::createSendThread(){
	tcpSendThread = new FSendDataToServerThread(mainLib, this, connectionID);
}


