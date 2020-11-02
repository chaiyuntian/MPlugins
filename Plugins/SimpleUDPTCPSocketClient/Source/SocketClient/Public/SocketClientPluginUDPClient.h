// Copyright 2017-2019 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"
#include "SocketClientPluginUDPClient.generated.h"


class USocketClientBPLibrary;
class FServerUDPConnectionThread;
class FServerUDPSendMessageThread;


UCLASS(Blueprintable, BlueprintType)
class USocketClientPluginUDPClient : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FsocketClientUDPConnectionEventDelegate, bool, success, FString, message, FString, clientConnectionID);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FreceiveUDPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray, FString, IP_FromSender, int32, portFromSender, FString, clientConnectionID);

	UFUNCTION()
		void socketClientUDPConnectionEventDelegate(const bool success, const FString message, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ConnectionInfo")
		FsocketClientUDPConnectionEventDelegate onsocketClientUDPConnectionEventDelegate;
	UFUNCTION()
		void receiveUDPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray, const  FString IP, const int32 port, const FString clientConnectionID);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events|ReceiveMessage")
		FreceiveUDPMessageEventDelegate onreceiveUDPMessageEventDelegate;

	void init(USocketClientBPLibrary* socketClientLibP, FString domain, int32 port, EReceiveFilterClient receiveFilter, FString clientConnectionID, int32 maxPacketSize = 65507);
	void sendUDPMessage(FString domainOrIP, int32 port, FString message, TArray<uint8> byteArray, FString clientConnectionID);
	void closeUDPConnection();
	void UDPReceiver(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);


	bool isRun();
	void setRun(bool runP);

	FSocket* getSocket();
	void setSocket(FSocket* socketP);

	void setUDPSocketReceiver(FUdpSocketReceiver* udpSocketReceiver);

	FString getIP();
	void setIP(FString ipP);
	int32 getPort();
	FString getDomainOrIP();

	FString getConnectionID();
	void setUDPSendThread(FServerUDPSendMessageThread* udpSendThreadP);

	int32 getMaxPacketSize();

private:
	bool run = false;
	EReceiveFilterClient receiveFilter;
	FString connectionID;
	FString domainOrIP;
	int32 port;
	int32 maxPacketSize = 65507;

	USocketClientBPLibrary* socketClientBPLibrary = nullptr;

	FUdpSocketReceiver* udpSocketReceiver = nullptr;
	FSocket* socket = nullptr;
	FServerUDPConnectionThread* UDPThread = nullptr;
	FServerUDPSendMessageThread* UDPSendThread = nullptr;
};

/* asynchronous Threads*/
class FServerUDPSendMessageThread : public FRunnable {

public:

	FServerUDPSendMessageThread(USocketClientPluginUDPClient* udpClientP, USocketClientBPLibrary* socketClientP, FString mySocketipP, int32 mySocketportP) :
		udpClient(udpClientP),
		socketClient(socketClientP),
		mySocketip(mySocketipP),
		mySocketport(mySocketportP) {
		FString threadName = "FServerUDPSendMessageThread_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {

		FSocket* socket = udpClient->getSocket();
		FString connectionID = udpClient->getConnectionID();
		maxPacketSize = udpClient->getMaxPacketSize();
		
		while (udpClient->isRun() && socket != nullptr) {


			if (udpClient->isRun() && (messageQueue.IsEmpty() == false || byteArrayQueue.IsEmpty() == false)) {

				TSharedRef<FInternetAddr> addr = USocketClientBPLibrary::getSocketSubSystem()->CreateInternetAddr();
				bool bIsValid;
				addr->SetIp(*sendToip, bIsValid);
				addr->SetPort(sendToport);
				int32 sent = 0;
				TArray<uint8> byteArray;
				if (bIsValid) {
					while (messageQueue.IsEmpty() == false) {
						FString m;
						messageQueue.Dequeue(m);
						FTCHARToUTF8 Convert(*m);
						byteArray.Append((uint8*)Convert.Get(), Convert.Length());
						sendBytes(socket, byteArray, sent, addr);
					}

					while (byteArrayQueue.IsEmpty() == false) {
						byteArrayQueue.Dequeue(byteArray);
						sendBytes(socket, byteArray, sent, addr);
					}

				}
				else {
					UE_LOG(LogTemp, Error, TEXT("Can't send to %s:%i"), *sendToip, sendToport);
				}

			}

			if (udpClient->isRun()) {
				pauseThread(true);
				//workaround. suspend do not work on all platforms. lets sleep
				while (paused && udpClient->isRun()) {
					FPlatformProcess::Sleep(0.01);
				}
			}
		}

		if (socket != nullptr) {
			socket->Close();
			socket = nullptr;
			udpClient->setSocket(nullptr);
		}


		USocketClientBPLibrary* socketClientTMP = socketClient;
		USocketClientPluginUDPClient* udpClientGlobal = udpClient;
		FString ipGlobal = mySocketip;
		int32 portGlobal = mySocketport;
		AsyncTask(ENamedThreads::GameThread, [socketClientTMP, udpClientGlobal, ipGlobal, portGlobal, connectionID]() {
			if (socketClientTMP != nullptr)
				socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "UDP connection closed. " + ipGlobal + ":" + FString::FromInt(portGlobal), connectionID);
			if (udpClientGlobal != nullptr)
				udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "UDP connection closed. " + ipGlobal + ":" + FString::FromInt(portGlobal), connectionID);
		});


		thread = nullptr;
		return 0;
	}


	void sendMessage(FString messageP, TArray<uint8> byteArrayP, FString ip, int32 port) {
		sendToip = socketClient->resolveDomain(ip);
		sendToport = port;
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

	void sendBytes(FSocket*& socketP, TArray<uint8>& byteArray, int32& sent, TSharedRef<FInternetAddr>& addr) {
		if (byteArray.Num() > maxPacketSize) {
			TArray<uint8> byteArrayTemp;
			for (int32 i = 0; i < byteArray.Num(); i++) {
				byteArrayTemp.Add(byteArray[i]);
				if (byteArrayTemp.Num() == maxPacketSize) {
					sent = 0;
					socketP->SendTo(byteArrayTemp.GetData(), byteArrayTemp.Num(), sent, *addr);
					byteArrayTemp.Empty();
				}
			}
			if (byteArrayTemp.Num() > 0) {
				sent = 0;
				socketP->SendTo(byteArrayTemp.GetData(), byteArrayTemp.Num(), sent, *addr);
				byteArrayTemp.Empty();
			}
		}
		else {
			sent = 0;
			socketP->SendTo(byteArray.GetData(), byteArray.Num(), sent, *addr);
		}

		byteArray.Empty();
	}

protected:
	USocketClientPluginUDPClient* udpClient = nullptr;
	USocketClientBPLibrary* socketClient = nullptr;
	FString mySocketip;
	int32 mySocketport;
	FString sendToip;
	int32 sendToport;
	FRunnableThread* thread = nullptr;
	bool					paused;
	TQueue<FString> messageQueue;
	TQueue<TArray<uint8>> byteArrayQueue;
	int32 maxPacketSize = 65507;
};


class FServerUDPConnectionThread : public FRunnable {

public:

	FServerUDPConnectionThread(USocketClientPluginUDPClient* udpClientP, USocketClientBPLibrary* socketClientP, FString ipP, int32 portP) :
		udpClient(udpClientP),
		socketClient(socketClientP),
		ipGlobal(ipP),
		portGlobal(portP) {
		FString threadName = "FServerUDPConnectionThread_" + FGuid::NewGuid().ToString();
		thread = FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {
		
			USocketClientPluginUDPClient* udpClientGlobal = udpClient;
			FString ip = socketClient->resolveDomain(ipGlobal);
			udpClient->setIP(ip);
			int32 port = portGlobal;
			FString connectionID = udpClient->getConnectionID();
					

			if (socket == nullptr || socket == NULL) {

				USocketClientBPLibrary* socketClientTMP = socketClient;

				FString endpointAdress = ip + ":" + FString::FromInt(port);
				FIPv4Endpoint Endpoint;

				// create the socket
				FString socketName;
				ISocketSubsystem* socketSubsystem = USocketClientBPLibrary::getSocketSubSystem();

				TSharedPtr<class FInternetAddr> addr = socketSubsystem->CreateInternetAddr();
				bool validIP = true;
				addr->SetPort(port);
				addr->SetIp(*ip, validIP);


				if (!validIP) {
					UE_LOG(LogTemp, Error, TEXT("SocketClient UDP. Can't set ip"));
					AsyncTask(ENamedThreads::GameThread, [udpClientGlobal,socketClientTMP, addr, connectionID]() {
						if (socketClientTMP != nullptr)
							socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "SocketClient UDP. Can't set ip", connectionID);
						if (udpClientGlobal != nullptr)
							udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "SocketClient UDP. Can't set ip", connectionID);
						});
					thread = nullptr;
					return 0;
				}

				socket = socketSubsystem->CreateSocket(NAME_DGram, *socketName, addr->GetProtocolType());


				if (socket == nullptr || socket == NULL) {
					const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
					UE_LOG(LogTemp, Error, TEXT("UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);
					AsyncTask(ENamedThreads::GameThread, [udpClientGlobal,socketClientTMP, addr, SocketErr, connectionID]() {
						if (socketClientTMP != nullptr)
							socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 0) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
						if (udpClientGlobal != nullptr)
							udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 0) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
					});
					thread = nullptr;
					return 0;
				}

				if (!socket->SetRecvErr()) {
					UE_LOG(LogTemp, Error, TEXT("SocketClient UDP. Can't set recverr"));
				}


				if (socket == nullptr || socket == NULL || !validIP) {
					const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
					UE_LOG(LogTemp, Error, TEXT("UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);
					AsyncTask(ENamedThreads::GameThread, [udpClientGlobal,socketClientTMP, addr, SocketErr, connectionID]() {
						if (socketClientTMP != nullptr)
							socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 1) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
						if (udpClientGlobal != nullptr)
							udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 1) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
						});
					thread = nullptr;
					return 0;
				}
				socket->SetReuseAddr(true);
				socket->SetNonBlocking(true);
				socket->SetBroadcast(true);

				if (!socket->Bind(*addr)) {
					const TCHAR* SocketErr = socketSubsystem->GetSocketError(SE_GET_LAST_ERROR_CODE);
					UE_LOG(LogTemp, Error, TEXT("UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router. %s:%i. Error: %s"), *ip, port, SocketErr);

					AsyncTask(ENamedThreads::GameThread, [udpClientGlobal,socketClientTMP, addr, SocketErr, connectionID]() {
						if (socketClientTMP != nullptr)
							socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 2) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
						if (udpClientGlobal != nullptr)
							udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(false, "(Error 2) UE4 could not init a UDP socket. You can only create listening connections on local IPs. An external IP must be redirected to a local IP in your router." + addr->ToString(true) + " Error:" + SocketErr, connectionID);
						});
					thread = nullptr;
					return 0;
				}



				FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
				FUdpSocketReceiver* udpSocketReceiver = new FUdpSocketReceiver(socket, ThreadWaitTime, TEXT("SocketClientBPLibUDPReceiverThread"));
				udpSocketReceiver->OnDataReceived().BindUObject(udpClient, &USocketClientPluginUDPClient::UDPReceiver);
				udpSocketReceiver->Start();
				udpClient->setUDPSocketReceiver(udpSocketReceiver);

				udpClient->setSocket(socket);
				udpClient->setRun(true);
				udpClient->setUDPSendThread(new FServerUDPSendMessageThread(udpClient, socketClient,ip,port));

	
				AsyncTask(ENamedThreads::GameThread, [udpClientGlobal,socketClientTMP, addr, connectionID]() {
					if (socketClientTMP != nullptr)
						socketClientTMP->onsocketClientUDPConnectionEventDelegate.Broadcast(true, "Init UDP Connection OK. " + addr->ToString(true), connectionID);
					if (udpClientGlobal != nullptr)
						udpClientGlobal->onsocketClientUDPConnectionEventDelegate.Broadcast(true, "Init UDP Connection OK. " + addr->ToString(true), connectionID);
				});

			}
		thread = nullptr;
		return 0;
	}




protected:
	USocketClientPluginUDPClient* udpClient = nullptr;
	USocketClientBPLibrary* socketClient = nullptr;
	FRunnableThread* thread = nullptr;
	FString					ipGlobal;
	int32					portGlobal;
	FSocket* socket = nullptr;
	bool reuseSocket = false;
};
