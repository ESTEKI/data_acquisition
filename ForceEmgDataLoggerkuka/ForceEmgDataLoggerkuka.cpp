
// ForceEmgDataLoggerkuka.cpp : Defines the entry point for the console application.
//

/*

// in case of Linker errors: goto Project-> Properties -> linker->input ->Additional library directory add(<edit>):ws2_32.lib || for UDP connection
// beware of Warnings! set to x64 platform unless using x86
*ERROR cannot open source file "myo/myo.hpp"
for compiler settings refer to Start Here page in myo sdk folder

also copy myo64.dll in system32 folder and yout project folder if that didn't work
i have disabled error number 4996 in settings c++ advanced...
*/



#include "stdafx.h"


#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fstream>
//#include <windows.h>
#include <array>
#include <cstdlib>
#include <cmath>
#include <conio.h>
#include <chrono>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <myo/myo.hpp>
#include <myo/libmyo.h>

#pragma comment(lib, "Ws2_32.lib")

#define TIMESTEP 1 //ms actual time step is bigger than this , it only sets the hub.run(ms) loop time
#define BUFFERSIZE 128
int elbowAngle = 904;

using namespace std;

SOCKET socketSend;
SOCKET socketRec;

struct sockaddr_in local;//udp send 
struct sockaddr_in otherMachine;
struct sockaddr_in localrec;//udp recieve socket different PORT number 
string fn;
int otherMachinelen = sizeof(otherMachine);
fd_set set;//this is for time out setting
//struct timeval timeout;//this is for time out setting 

class DataCollector : public myo::DeviceListener {

	fstream logFile;
public:
	DataCollector()
		: emgSamples()
	{
		logFile.open(  "logFile" + to_string(elbowAngle) + "deg.txt", std::fstream::out);//warning this will rewrite text file and erase previous recordings
		logFile << "EMG1  EMG2  EMG3  EMG4  EMG5  EMG6  EMG7  EMG8  ForceX ForceY ForceZ tauX tawY tayZ Angle_deg TimeStep_ms\n";
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void print(string kukaForces)
	{

		// Clear the current line
		std::cout << '\r';
		std::string EMGs;
		// Print out the EMG data.
		for (int8_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();

			//std::cout << ' ' << emgString << std::string(4 - emgString.size(), ' ') << ' ';

			EMGs = EMGs + ' ' + emgString + std::string(4 - emgString.size(), ' ') + ' ';

		}
		//std::cout   << angle;
		EMGs = EMGs + kukaForces + ' ' + '0' + '\n';
		logFile << EMGs;
		//cout << EMGs;

		//std::cout << std::flush;
	}

	// The values of this array is set by onEmgData() above.
	std::array<int8_t, 8> emgSamples;
	
};


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	otherMachine.sin_family = AF_INET;
	otherMachine.sin_port = htons(30002);
	//otherMachine.sin_addr.s_addr = inet_addr("172.31.1.147");//kuka ip
	InetPton(AF_INET, _T("172.31.1.147"), &otherMachine.sin_addr.s_addr);//kuka ip

	int otherMashinelen = sizeof(otherMachine);

	local.sin_family = AF_INET;
	local.sin_port = htons(50003);
	//local.sin_addr.s_addr = inet_addr("172.31.1.150");// pc 
	InetPton(AF_INET, _T("172.31.1.150"), &local.sin_addr.s_addr);

	localrec.sin_family = AF_INET;
	localrec.sin_port = htons(50004);
	//local.sin_addr.s_addr = inet_addr("172.31.1.150");// pc 
	InetPton(AF_INET, _T("172.31.1.150"), &localrec.sin_addr.s_addr);

	socketSend = socket(AF_INET, SOCK_DGRAM, 0);
	bind(socketSend, (sockaddr*)&local, sizeof(local));

	socketRec = socket(AF_INET, SOCK_DGRAM, 0);
	bind(socketRec, (sockaddr*)&localrec, sizeof(localrec));

	 
	
	
	FD_ZERO(&set); /* clear the set *///this is for time out setting 
	FD_SET(socketSend, &set); /* add our file descriptor to the set *///this is for time out setting 
	DWORD recTimeOut = 500;
	setsockopt(socketRec, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recTimeOut, sizeof(recTimeOut));
}

void sendViaUDP(string msg)
{
	char sbuffer[BUFFERSIZE];//u have to use same buffer size on both sides (pc and kuka)!!!!!!!!!!!
	strcpy(sbuffer, msg.c_str());
	sendto(socketSend, sbuffer, sizeof(sbuffer), 0, (sockaddr*)&otherMachine, otherMachinelen);
	ZeroMemory(sbuffer, sizeof(sbuffer));
}

string recieveViaUDP()
{
	char rbuffer[BUFFERSIZE];
	ZeroMemory(rbuffer, sizeof(rbuffer));
	
	int rv = 1;// select(socketS, &set, NULL, NULL, &timeout);//this will wait until timeout occures...
	//select(...) checks if there is any data ready BUUUUUTT it makes the connection vulnerable so i comented it ans use setsockop(..) instead
	if (rv == SOCKET_ERROR)
	{
		printf("select error");
	}
	else if (rv == 0)
	{
		printf("timeout, socket does not have anything to read");
		// timeout, socket does not have anything to read
	}
	else
	{
		// socket has something to read
		int recv_size = recv(socketRec, rbuffer, sizeof(rbuffer), 0);
		if (recv_size == SOCKET_ERROR)
		{
			// read failed...
			printf("read from UDP connection failed\n");
		}
		else if (recv_size == 0)
		{
			// peer disconnected...
			printf("peer disconnected");
		}
		else
		{
			// read successful...

			//printf("t");
			//printf("\n");
			
		}
	}

	return rbuffer;
}

int main(int argc, char** argv)
{
	int iteration = 0;
	InitWinsock();
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {
		string kukaData;
		fstream timeFile;
		timeFile.open("timeFile"+ to_string(elbowAngle)+".txt", std::fstream::out);
		timeFile << "TotalElapsedTime_ms\n";
		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.example.emg-data-sample");

		std::cout << "Attempting to find a Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		myo::Myo* myo = hub.waitForMyo(10000);

		// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}

		// We've found a Myo.
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

		// Next we enable EMG streaming on the found Myo.
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);

		myo->vibrate(myo->vibrationShort);
		myo->lock();

		cout << "Press 'S' to start recording\npress any key to exit.";
		char key = getch();
		//cout << key;
		//SHORT keyState = GetKeyState(VK_RETURN);//enter key
		bool rec = false;
		if (key == 's') {

			for (int c = 4; c > 0; c--)
			{
				cout << "program  will start in " << c << " seconds\n";
				Sleep(1000);
			}

			cout << "\n\nRECORDING STARTED .....!!!!!press and hold ESC to end recording ";
			rec = true;
		}
		//// Finally we enter our main loop.
		Sleep(500);
		SHORT keyState = GetKeyState(VK_ESCAPE);//there was a bug reading this so here we read escape toggle to see if it stuck somwhere in the buffer or smt ... never mind
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		sendViaUDP(to_string(elbowAngle) + "\n");//starting command for while loop in kuka
		
		Sleep(500);
		// Finally we enter our main loop.
		while (rec == true && iteration < 3800 ) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
			sendViaUDP("2\n");
			hub.run(4);
			//Sleep(800);
			kukaData =  recieveViaUDP() ;
			// After processing events, we call the print() member function we defined above to print out the values we've
			// obtained from any events that have occurred.
			collector.print(kukaData);

			keyState = GetKeyState(VK_ESCAPE);
			bool isDown = keyState & 0x8000;
			if (isDown) {// check if escape key is pressed
				rec = false;
			}
			iteration++;
			cout << "\r" << to_string(iteration) << "        ";
		}
		sendViaUDP("3\n");
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		long long elapsedtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
		timeFile << elapsedtime;

		closesocket(socketSend);
		closesocket(socketRec);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}
