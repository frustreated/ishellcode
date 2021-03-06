// ishellcode.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
using namespace boost::program_options;
#include "ProcManager.h"
#include "instdrv.h"

typedef struct tagKeyBUF
{
	DWORD_PTR uMyPid;
	DWORD_PTR uTargetPid;
	DWORD_PTR pShellAddr;
	DWORD_PTR uShellSize;
}KEYBUF, *PKEYBUF;

int main(int argc, char* argv[])
{

	try
	{
		DWORD dwPid;
		int nType;
		std::string strShellPath;
		options_description desc{ "Options" };
		desc.add_options()
			("help,h", "produce help message")
			("pid,p", value<DWORD>(&dwPid), "target pid")
			("type,t", value<int>(&nType)->default_value(0), "0:r3 1:r0")
			("shell,s", value<std::string>(&strShellPath), "shellcode binary path");
		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);

		if (vm.count("help"))
			std::cout << desc << '\n';
		else if (vm.count("pid") && vm.count("shell") && vm.count("type"))
		{	
			
			ProcManager::EnableDebugPriv();

			HANDLE hFile = CreateFile(strShellPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				std::cout << "open file failed\n";
				return 1;
			}
			DWORD dwShellSize = GetFileSize(hFile, 0);
			DWORD dwReaded;
			BYTE *bShell = new BYTE[dwShellSize];
			ReadFile(hFile, bShell, dwShellSize, &dwReaded, NULL);
			CloseHandle(hFile);

			if (nType == 0)
			{
				if (ProcManager::InjectShellcode(dwPid, bShell, dwShellSize))
				{
					std::cout << "user inject ok\n";
				}
			}
			else if (nType==1)
			{
				HANDLE hDevice = NULL;
				std::string strDrvPath = (boost::filesystem::initial_path<boost::filesystem::path>()/"TestWDK.sys").string();
				BOOL bRet = scmLoadDeviceDriver("ishellcode", strDrvPath.c_str(), &hDevice);
				if (!bRet || hDevice == NULL)
				{
					std::cout << "init driver failed\n"<<GetLastError();
					return 1;
				}

				KEYBUF kb = { 0 };
				kb.uMyPid = GetCurrentProcessId();
				kb.uTargetPid = dwPid;
				kb.pShellAddr = (DWORD_PTR)bShell;
				kb.uShellSize = dwShellSize;
				DWORD dwWrited;
				WriteFile(hDevice, &kb, sizeof(kb), &dwWrited, NULL);
				scmUnloadDeviceDriver("ishellcode");
				CloseHandle(hDevice);
				std::cout << "kernel inject ok\n";
			}			
		}
		else 
		{
			std::cout << desc << '\n';
		}

	}
	catch (const error &ex)
	{
		std::cerr << ex.what() << '\n';
	}
	return 0;
}

