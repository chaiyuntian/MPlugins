// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"
#include "SocketClientPluginTCPClient.generated.h"


class USocketClientBPLibrary;
class FServerConnectionThread;
class FSendDataToServerThread;

UCLASS(Blueprintable, BlueprintType)
class USocketClientPluginTCPClient : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientTCPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FreceiveTCPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, clientConnectionID);

	UFUNCTION()
		void socketClientTCPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|ConnectionInfo")
		FsocketClientTCPConnectionEventDelegate onsocketClientTCPConnectionEventDelegate;
	UFUNCTION()
		void receiveTCPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events|ReceiveMessage")
		FreceiveTCPMessageEventDelegate onreceiveTCPMessageEventDelegate;

	void connect(USocketClientBPLibrary* mainLib, FString domainOrIP, int32 port, EReceiveFilterClient receiveFilter, FString connectionID);
	void sendMessage(FString message, TArray<uint8> byteArray);
	void closeConnection();

	bool isRun();
	void setRun(bool runP);
	FString getConnectionID();

	void setSocket(FSocket* socket);
	FSocket* getSocket();

	void createSendThread();
	

private:
	bool run = false;
	FString connectionID;
	FString domainOrIP;
	int32 port;
	FSocket* socket = nullptr;

	FServerConnectionThread* tcpConnectionThread = nullptr;
	FSendDataToServerThread* tcpSendThread = nullptr;
	USocketClientBPLibrary* mainLib = nullptr;

};

/* asynchronous Thread*/
class FServerConnectionThread : public FRunnable {

public:

	FServerConnectionThread(USocketClientBPLibrary* socketClientP, FString clientConnectionIDP, EReceiveFilterClient receiveFilterP, FString ipOrDomainP, int32 portP, USocketClientPluginTCPClient* tcpClientP) :
		socketClient(socketClientP),
		clientConnectionID(clientConnectionIDP),
		receiveFilter(receiveFilterP),
		ipOrDomain(ipOrDomainP),
		port(portP),
		tcpClient(tcpClientP){
		FString threadName = "FServerConnectionThread" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {
		//UE_LOG(LogTemp, Display, TEXT("DoWork:%s"),*(FDateTime::Now()).ToString());
		FString ip = socketClient->resolveDomain(ipOrDomain);
		int32 portGlobal = port;
		FString clientConnectionIDGlobal = clientConnectionID;
		USocketClientBPLibrary* socketClientGlobal = socketClient;
		USocketClientPluginTCPClient* tcpClientGlobal = tcpClient;

		//message wrapping
		FString tcpMessageHeader;
		FString tcpMessageFooter;
		bool useTCPMessageWrapping;

		socketClient->getTcpMessageWrapping(tcpMessageHeader, tcpMessageFooter, useTCPMessageWrapping);

		FString tcpMessageFooterLineBreak = tcpMessageFooter+"\r\n";
		FString tcpMessageFooterLineBreak2 = tcpMessageFooter + "\r";;


		//UE_LOG(LogTemp, Warning, TEXT("Tread:%s:%i"),*ip, port);
		ISocketSubsystem* sSS = USocketClientBPLibrary::getSocketSubSystem();
		if (sSS == nullptr) {
			const TCHAR* socketErr = sSS->GetSocketError(SE_GET_LAST_ERROR_CODE);
			AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, socketClientGlobal, tcpClientGlobal, socketErr]() {
				if (socketClientGlobal != nullptr)
					socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(1). SocketSubSystem does not exist:" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				if (tcpClientGlobal != nullptr)
					tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(1). SocketSubSystem does not exist:" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				});
			return 0;
		}
		TSharedRef<FInternetAddr> addr = sSS->CreateInternetAddr();
		bool bIsValid;
		addr->SetIp(*ip, bIsValid);
		addr->SetPort(port);

		if (bIsValid) {
			// create the socket
			FSocket* socket = sSS->CreateSocket(NAME_Stream, TEXT("socketClient"), addr->GetProtocolType());
			tcpClient->setSocket(socket);
;

			// try to connect to the server
			if (socket == nullptr || socket->Connect(*addr) == false) {
				const TCHAR* socketErr = sSS->GetSocketError(SE_GET_LAST_ERROR_CODE);
				AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, socketClientGlobal, tcpClientGlobal, socketErr]() {
					if (socketClientGlobal != nullptr)
						socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(2):" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					if (tcpClientGlobal != nullptr)
						tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(2):" + FString(socketErr) + "|" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					});
			}
			else {
				AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
					if (socketClientGlobal != nullptr)
						socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(true, "Connection successful:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					if (tcpClientGlobal != nullptr)
						tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(true, "Connection successful:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					});
				tcpClient->setRun(true);
				tcpClient->createSendThread();
				int64 ticks1;
				int64 ticks2;
				TArray<uint8> byteDataArray;
				FString mainMessage;
				bool inCollectMessageStatus = false;

				uint32 DataSize;
				while (socket != nullptr && tcpClient->isRun()) {

					//ESocketConnectionState::SCS_Connected does not work https://issues.unrealengine.com/issue/UE-27542
					//Compare ticks is a workaround to get a disconnect. clientSocket->Wait() stop working after disconnect. (Another bug?)
					//If it doesn't wait any longer, ticks1 and ticks2 should be the same == disconnect.
					ticks1 = FDateTime::Now().GetTicks();
					socket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan::FromSeconds(0.1));
					ticks2 = FDateTime::Now().GetTicks();

					bool hasData = socket->HasPendingData(DataSize);
					if (!hasData && ticks1 == ticks2) {
						UE_LOG(LogTemp, Display, TEXT("TCP connection broken. End Loop"));
						break;
					}

					if (hasData) {
						FArrayReaderPtr Datagram = MakeShareable(new FArrayReader(true));
						Datagram->SetNumUninitialized(DataSize);
						int32 BytesRead = 0;
						if (socket->Recv(Datagram->GetData(), Datagram->Num(), BytesRead)) {

							if (useTCPMessageWrapping) {
								if (receiveFilter == EReceiveFilterClient::E_SAB || receiveFilter == EReceiveFilterClient::E_S) {
									char* Data = (char*)Datagram->GetData();
									Data[BytesRead] = '\0';

									FString recvMessage = FString(UTF8_TO_TCHAR(Data));
									if (recvMessage.StartsWith(tcpMessageHeader)) {
										inCollectMessageStatus = true;
										recvMessage.RemoveFromStart(tcpMessageHeader);
									}
									if (recvMessage.EndsWith(tcpMessageFooter) || recvMessage.EndsWith(tcpMessageFooterLineBreak) || recvMessage.EndsWith(tcpMessageFooterLineBreak2)) {
										inCollectMessageStatus = false;
										if (!recvMessage.RemoveFromEnd(tcpMessageFooter)) {
											if (!recvMessage.RemoveFromEnd(tcpMessageFooterLineBreak)) {
												if (recvMessage.RemoveFromEnd(tcpMessageFooterLineBreak2)) {
													recvMessage.Append("\r");
												}
											}
											else {
												recvMessage.Append("\r\n");
											}
										}

										//splitt merged messages
										if (recvMessage.Contains(tcpMessageHeader)) {
											TArray<FString> lines;
											int32 lineCount = recvMessage.ParseIntoArray(lines, *tcpMessageHeader, true);
											for (int32 i = 0; i < lineCount; i++) {
												mainMessage = lines[i];
												if (mainMessage.EndsWith(tcpMessageFooter) || mainMessage.EndsWith(tcpMessageFooterLineBreak) || mainMessage.EndsWith(tcpMessageFooterLineBreak2)) {
													if (!mainMessage.RemoveFromEnd(tcpMessageFooter)) {
														if (!mainMessage.RemoveFromEnd(tcpMessageFooterLineBreak)) {
															if (mainMessage.RemoveFromEnd(tcpMessageFooterLineBreak2)) {
																mainMessage.Append("\r");
															}
														}
														else {
															mainMessage.Append("\r\n");
														}
													}
												}

												//switch to gamethread
												AsyncTask(ENamedThreads::GameThread, [mainMessage, byteDataArray, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
													if (socketClientGlobal != nullptr)
														socketClientGlobal->onreceiveTCPMessageEventDelegate.Broadcast(mainMessage, byteDataArray, clientConnectionIDGlobal);
													if (tcpClientGlobal != nullptr)
														tcpClientGlobal->onreceiveTCPMessageEventDelegate.Broadcast(mainMessage, byteDataArray, clientConnectionIDGlobal);
													});

												Datagram->Empty();
												mainMessage.Empty();
											}
											continue;
										}
										else {
											mainMessage.Append(recvMessage);
										}


									}
									if (inCollectMessageStatus) {
										mainMessage.Append(recvMessage);
										continue;
									}
									if (mainMessage.IsEmpty()) {
										continue;
									}

								}

							}
							else {

								if (receiveFilter == EReceiveFilterClient::E_SAB || receiveFilter == EReceiveFilterClient::E_S) {
									char* Data = (char*)Datagram->GetData();
									Data[BytesRead] = '\0';
									mainMessage = FString(UTF8_TO_TCHAR(Data));
								}
								
								if (receiveFilter == EReceiveFilterClient::E_SAB || receiveFilter == EReceiveFilterClient::E_B) {
									byteDataArray.Append(Datagram->GetData(), Datagram->Num());
								}
							}



							

							//switch to gamethread
							AsyncTask(ENamedThreads::GameThread, [mainMessage, byteDataArray, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
								if (socketClientGlobal != nullptr)
									socketClientGlobal->onreceiveTCPMessageEventDelegate.Broadcast(mainMessage, byteDataArray, clientConnectionIDGlobal);
								if (tcpClientGlobal != nullptr)
									tcpClientGlobal->onreceiveTCPMessageEventDelegate.Broadcast(mainMessage, byteDataArray, clientConnectionIDGlobal);
								});
						}
						mainMessage.Empty();
						byteDataArray.Empty();
						Datagram->Empty();
					}
				}


				AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
					if (socketClientGlobal != nullptr) {
						socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection close:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					if (tcpClientGlobal != nullptr)
						tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection close:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
					}

				});
			}

			//wait for game thread
			//FPlatformProcess::Sleep(1);

			if (tcpClient->isRun()) {
				USocketClientBPLibrary::getSocketClientTarget()->closeSocketClientConnectionTCP(clientConnectionID);
			}

			
			tcpClient->setRun(false);
			if (socket != nullptr) {
				socket->Close();
				socket = nullptr;
				sSS->DestroySocket(socket);
			}
			thread = nullptr;
		}
		else {
			AsyncTask(ENamedThreads::GameThread, [ip, portGlobal, clientConnectionIDGlobal, tcpClientGlobal, socketClientGlobal]() {
				if (socketClientGlobal != nullptr)
					socketClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(3). IP not valid:" + ip+ ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				if (tcpClientGlobal != nullptr)
					tcpClientGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection failed(3). IP not valid:" + ip + ":" + FString::FromInt(portGlobal) + "|" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				});
		}

		return 0;
	}


protected:
	USocketClientBPLibrary* socketClient = nullptr;
	//USocketClientBPLibrary*		oldClient;
	FString						clientConnectionID;
	FString						originalIP;
	EReceiveFilterClient		receiveFilter;
	FString ipOrDomain;
	int32 port;
	USocketClientPluginTCPClient* tcpClient = nullptr;
	FRunnableThread* thread = nullptr;
};




/* asynchronous Thread*/
class FSendDataToServerThread : public FRunnable {

public:

	FSendDataToServerThread(USocketClientBPLibrary* socketClientLibP, USocketClientPluginTCPClient* tcpClientP, FString clientConnectionIDP) :
		socketClientLib(socketClientLibP),
		tcpClient(tcpClientP),
		clientConnectionID(clientConnectionIDP) {
		FString threadName = "FSendDataToServerThread" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {

		if (tcpClient == nullptr) {
			UE_LOG(LogTemp, Error, TEXT("Class is not initialized."));
			return 0;
		}

		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, TEXT("tcp socket 1"));*/

		FString clientConnectionIDGlobal = clientConnectionID;
		//USocketClientPluginTCPClient* tcpClientGlobal = tcpClient;
		USocketClientBPLibrary* socketClientLibGlobal = socketClientLib;

		// get the socket
		FSocket* socket = tcpClient->getSocket();

		while (tcpClient->isRun()) {

			// try to connect to the server
			if (socket == NULL || socket == nullptr) {
				UE_LOG(LogTemp, Error, TEXT("Connection not exist."));
				//AsyncTask(ENamedThreads::GameThread, [clientConnectionIDGlobal, socketClientLibGlobal]() {
				//	if (socketClientLibGlobal != nullptr) {
				//		socketClientLibGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection not exist:" + clientConnectionIDGlobal, clientConnectionIDGlobal);
				//		socketClientLibGlobal->closeSocketClientConnection();
				//	}
				//});
				break;
			}

			if (socket != nullptr && socket->GetConnectionState() == ESocketConnectionState::SCS_Connected) {
				while (messageQueue.IsEmpty() == false) {
					FString m;
					messageQueue.Dequeue(m);
					FTCHARToUTF8 Convert(*m);
					int32 sent = 0;
					socket->Send((uint8*)((ANSICHAR*)Convert.Get()), Convert.Length(), sent);
				}

				while (byteArrayQueue.IsEmpty() == false) {
					TArray<uint8> ba;
					byteArrayQueue.Dequeue(ba);
					int32 sent = 0;
					socket->Send(ba.GetData(), ba.Num(), sent);
					ba.Empty();
				}

			}
			//else {
			//	UE_LOG(LogTemp, Error, TEXT("Connection Lost"));
			//	AsyncTask(ENamedThreads::GameThread, [clientConnectionIDGlobal, socketClientLibGlobal]() {
			//		if (socketClientLibGlobal != nullptr)
			//			socketClientLibGlobal->onsocketClientTCPConnectionEventDelegate.Broadcast(false, "Connection lost:" + clientConnectionIDGlobal, clientConnectionIDGlobal);
			//		});
			//}
			if (tcpClient->isRun()) {
				pauseThread(true);
				//workaround. suspend do not work on all platforms. lets sleep
				while (paused && tcpClient->isRun()) {
					FPlatformProcess::Sleep(0.01);
				}
			}
		}
		thread = nullptr;
		return 0;
	}


	void sendMessage(FString messageP, TArray<uint8> byteArrayP) {
		if (messageP.Len() > 0)
			messageQueue.Enqueue(messageP);
		if (byteArrayP.Num() > 0)
			byteArrayQueue.Enqueue(byteArrayP);
		pauseThread(false);
	}


	void pauseThread(bool pause) {
		paused = pause;
		if (thread != nullptr)
			thread->Suspend(pause);
	}


protected:
	TQueue<FString> messageQueue;
	TQueue<TArray<uint8>> byteArrayQueue;
	USocketClientBPLibrary* socketClientLib;
	USocketClientPluginTCPClient* tcpClient = nullptr;
	FString clientConnectionID;
	FRunnableThread* thread = nullptr;
	bool					run;
	bool					paused;
};