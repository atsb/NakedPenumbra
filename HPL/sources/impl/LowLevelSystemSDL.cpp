/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
//#include <vld.h>
//Use this to check for memory leaks!

#ifdef _WIN32
#pragma comment(lib, "angelscript.lib")
#define UNICODE
#include <windows.h>
#include <ShlObj.h>
#endif

#define _UNICODE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <fstream>
#include <string>

#include "impl/LowLevelSystemSDL.h"

#include <SDL2/SDL.h>

#include "system/String.h"

#ifndef _WIN32
#include <clocale>
#include <langinfo.h>
#include <unistd.h>
#endif

namespace hpl {
	// macOS version defined in LowLevelSystemMac.mm
	tString GetDataDir();

#ifdef __linux__
	tString GetDataDir()
	{
		tString temp;
		BrInitError error;
		if (!br_init (&error)) {
			// Log non-fatal error
			printf("*** BinReloc failed to initialize. Error: %d\n", error);
		} else {
			char *exedir;
			exedir = br_find_exe_dir(NULL);
			if (exedir) {
				temp = exedir;
				free(exedir);
			}
		}
		return temp;
	}
#endif
} // ns hpl

extern int hplMain(const hpl::tString &asCommandLine);

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(	HINSTANCE hInstance,  HINSTANCE hPrevInstance,LPSTR	lpCmdLine, int nCmdShow)
{
	return hplMain(lpCmdLine);
}
#else
int main(int argc, char *argv[])
{
#ifdef __linux__
	if(!std::setlocale(LC_CTYPE, "")) {
		fprintf(stderr, "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL.\n");
		return 1;
	}
	char *charset = nl_langinfo(CODESET);
	bool utf8_mode = (strcasecmp(charset, "UTF-8") == 0);
	if (!utf8_mode) {
		fprintf(stderr, "UTF-8 Charset %s available.\nCurrent LANG is %s\nCharset: %s\n",
			utf8_mode ? "is" : "not",
			getenv("LANG"), charset);
	}
#endif

	bool cwd = false;
	hpl::tString cmdline = "";
	for (int i=1; i < argc; i++) {
		if (strcmp(argv[i], "-cwd") == 0) {
			cwd = true;
		} else if (strncmp(argv[i], "-psn", 4) == 0) {
			// skip "finder" process number
		} else {
			if (cmdline.length()>0) {
				cmdline.append(" ").append(argv[i]);
			} else {
				cmdline.append(argv[i]);
			}
		}
	}

	if (!cwd) {
		hpl::tString dataDir = hpl::GetDataDir();

		chdir(dataDir.c_str());
	}

	return hplMain(cmdline);
}
#endif


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// LOG WRITER
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	static cLogWriter gLogWriter(_W("hpl.log"));
	static cLogWriter gUpdateLogWriter(_W("hpl_update.log"));

	//-----------------------------------------------------------------------

	cLogWriter::cLogWriter(const tWString& asFileName)
	{
		msFileName = asFileName;
	}

	cLogWriter::~cLogWriter()
	{
		if(mpFile) fclose(mpFile);
	}

	void cLogWriter::Write(const tString& asMessage)
	{
		if(!mpFile) ReopenFile();

		if(mpFile)
		{
			fprintf(mpFile, "%s", asMessage.c_str());
			fflush(mpFile);
		}
	}

	void cLogWriter::Clear()
	{
		ReopenFile();
		if(mpFile) fflush(mpFile);
	}

	//-----------------------------------------------------------------------


	void cLogWriter::SetFileName(const tWString& asFile)
	{
		if(msFileName == asFile) return;

		msFileName = asFile;
		ReopenFile();
	}

	//-----------------------------------------------------------------------


	void cLogWriter::ReopenFile()
	{
		if(mpFile) fclose(mpFile);


		#ifdef _WIN32
			mpFile = _wfopen(msFileName.c_str(),_W("w"));
		#else
			mpFile = fopen(cString::To8Char(msFileName).c_str(),"w");
		#endif
	}


	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelSystemSDL::cLowLevelSystemSDL()
	{
		Log("-------- THE HPL ENGINE LOG ------------\n\n");
	}

	//-----------------------------------------------------------------------

	cLowLevelSystemSDL::~cLowLevelSystemSDL()
	{
		//perhaps not the best thing to skip :)
		//if(gpLogWriter)	hplDelete(gpLogWriter);
		//gpLogWriter = NULL;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void SetLogFile(const tWString &asFile)
	{
		gLogWriter.SetFileName(asFile);
	}

	//-----------------------------------------------------------------------

#ifdef __MACOSX__
	void MacOSAlertBox(eMsgBoxType eType, tString caption, tString message);
#endif

// [ZM] define this to have all log, warning and error messages go to stdout/stderr as well
#define STD_LOGGING

	void FatalError(const char* fmt,... )
	{
		char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
			vsprintf(text, fmt, ap);
		va_end(ap);

#ifdef STD_LOGGING
		fprintf(stderr, "FATAL ERROR: %s", text);
#else
		tString sMess = "FATAL ERROR: ";
		sMess += text;
		gLogWriter.Write(sMess);
#endif
#ifdef _WIN32
		MessageBox( NULL, cString::To16Char(text).c_str(), _W("FATAL ERROR"), MB_ICONERROR);
#elif __MACOSX__
		MacOSAlertBox(eMsgBoxType_Error, sMess, "");
#endif

		exit(1);
	}

	void Error(const char* fmt, ...)
	{
		char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
			vsprintf(text, fmt, ap);
		va_end(ap);

#ifdef STD_LOGGING
		fprintf(stderr, "ERROR: %s", text);
#endif
		tString sMess = "ERROR: ";
		sMess += text;
		gLogWriter.Write(sMess);

	}

	void Warning(const char* fmt, ...)
	{
		char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
			vsprintf(text, fmt, ap);
		va_end(ap);

#ifdef STD_LOGGING
		fprintf(stdout, "WARNING: %s", text);
#endif
		tString sMess = "WARNING: ";
		sMess += text;
		gLogWriter.Write(sMess);
	}

	void Log(const char* fmt, ...)
	{
		char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		va_end(ap);
		
#ifdef STD_LOGGING
		fprintf(stdout, "%s", text);
#endif
		tString sMess = "";
		sMess += text;
		gLogWriter.Write(sMess);
	}

	//-----------------------------------------------------------------------

	static bool gbUpdateLogIsActive;
	void SetUpdateLogFile(const tWString &asFile)
	{
		gUpdateLogWriter.SetFileName(asFile);
	}

	void ClearUpdateLogFile()
	{
		if(!gbUpdateLogIsActive) return;

		gUpdateLogWriter.Clear();
	}

	void SetUpdateLogActive(bool abX)
	{
		gbUpdateLogIsActive =abX;
	}

	void LogUpdate(const char* fmt, ...)
	{
		if(!gbUpdateLogIsActive) return;

		char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		va_end(ap);

		tString sMess = "";
		sMess += text;
		gUpdateLogWriter.Write(sMess);
	}

	//-----------------------------------------------------------------------

	void CopyTextToClipboard(const tWString &asText)
	{
#ifdef _WIN32
		OpenClipboard(NULL);
		EmptyClipboard();

		HGLOBAL clipbuffer;
		wchar_t * pBuffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, (asText.size()+1) *sizeof(wchar_t));
		pBuffer = (wchar_t*)GlobalLock(clipbuffer);
		wcscpy(pBuffer, asText.c_str());
		//GlobalUnlock(clipbuffer);

		SetClipboardData(CF_UNICODETEXT, clipbuffer);

		GlobalUnlock(clipbuffer);

		CloseClipboard();
#endif
	}

	tWString LoadTextFromClipboard()
	{
#ifdef _WIN32
		tWString sText=_W("");
		OpenClipboard(NULL);

		HGLOBAL clipbuffer = GetClipboardData(CF_UNICODETEXT);

		wchar_t *pBuffer = (wchar_t*)GlobalLock(clipbuffer);

		if(pBuffer != NULL) sText = pBuffer;

		GlobalUnlock(clipbuffer);

		CloseClipboard();

		return sText;
#else
		return _W("");
#endif
	}

	//-----------------------------------------------------------------------


	void CreateMessageBox ( eMsgBoxType eType, const char* asCaption, const char *fmt, ...)
	{
		/*char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		va_end(ap);

		tString sMess = "";
		sMess += text;


		#ifdef WIN32

		UINT lType = MB_OK;

		switch (eType)
		{
		case eMsgBoxType_Info:
			lType += MB_ICONINFORMATION;
			break;
		case eMsgBoxType_Error:
			lType += MB_ICONERROR;
			break;
		case eMsgBoxType_Warning:
			lType += MB_ICONWARNING;
			break;
		default:
			break;
		}


		MessageBox( NULL, sMess.c_str(), asCaption, lType );

		#endif*/
	}

	void CreateMessageBox ( const char* asCaption, const char *fmt, ...)
	{
		/*char text[2048];
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		va_end(ap);

		tString sMess = "";
		sMess += text;

		CreateMessageBox( eMsgBoxType_Default, asCaption, sMess.c_str() );*/
	}


	void CreateMessageBoxW (eMsgBoxType eType, const wchar_t* asCaption, const wchar_t* fmt, va_list ap)
	{
		wchar_t text[2048];

		if (fmt == NULL)
			return;
		vswprintf(text, 2047, fmt, ap);

		tWString sMess = _W("");

		#ifdef _WIN32
		sMess += text;

		UINT lType = MB_OK;

		switch (eType)
		{
		case eMsgBoxType_Info:
			lType += MB_ICONINFORMATION;
			break;
		case eMsgBoxType_Error:
			lType += MB_ICONERROR;
			break;
		case eMsgBoxType_Warning:
			lType += MB_ICONWARNING;
			break;
		default:
			break;
		}

		MessageBox( NULL, sMess.c_str(), asCaption, lType );
		#else
		sMess += asCaption;
		sMess +=_W("\n\n");
		sMess += text;

		// TODO: show system alert

		#endif
	}

	void CreateMessageBoxW ( eMsgBoxType eType, const wchar_t* asCaption, const wchar_t* fmt, ...)
	{
		va_list ap;

		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		CreateMessageBoxW (eType, asCaption, fmt, ap);
		va_end(ap);
	}

	void CreateMessageBoxW ( const wchar_t* asCaption, const wchar_t *fmt, ...)
	{
		va_list ap;
		if (fmt == NULL)
			return;
		va_start(ap, fmt);
		CreateMessageBoxW( eMsgBoxType_Default, asCaption, fmt, ap );
		va_end(ap);
	}

	//-----------------------------------------------------------------------

	static cDate DateFromGMTIme(struct tm* apClock)
	{
		cDate date;

		date.seconds = apClock->tm_sec;
		date.minutes = apClock->tm_min;
		date.hours = apClock->tm_hour;
		date.month_day = apClock->tm_mday;
		date.month = apClock->tm_mon;
		date.year = 1900 + apClock->tm_year;
		date.week_day = apClock->tm_wday;
		date.year_day = apClock->tm_yday;

		return date;
	}

	//-----------------------------------------------------------------------

	void OpenBrowserWindow ( const tWString& asURL )
	{
		#ifdef _WIN32
		ShellExecute ( NULL, _W("open"), asURL.c_str(), NULL, NULL, SW_SHOWNORMAL );
		#elif defined(__linux__)
		tString asTemp = "./openurl.sh "+cString::To8Char(asURL);
		system(asTemp.c_str());
		#elif defined(__APPLE__)
		tString asTemp = "open "+cString::To8Char(asURL);
		system(asTemp.c_str());
		#endif
	}

	//-----------------------------------------------------------------------

	tWString GetSystemSpecialPath(eSystemPath aPathType)
	{
	#if defined(_WIN32)
		int type;
		switch(aPathType)
		{
		case eSystemPath_Personal:	type = CSIDL_PERSONAL;
			break;
		default: return _W("");
		}

		TCHAR sPath[1024];
		if(SUCCEEDED(SHGetFolderPath(NULL,
			type | CSIDL_FLAG_CREATE,
			NULL,0,sPath)))
		{
			return tWString(sPath);
		}
		else
		{
			return _W("");
		}
	#else
		switch (aPathType)
		{
		case eSystemPath_Personal: {
			const char *home = getenv("HOME");
			return cString::To16Char(tString(home));
		}
		default:
			return _W("");
		}
	#endif
	}

	//-----------------------------------------------------------------------

	bool FileExists(const tWString& asFileName)
	{
	#ifdef _WIN32
		FILE *f = _wfopen(asFileName.c_str(),_W("r"));
	#else
		FILE *f = fopen(cString::To8Char(asFileName).c_str(),"r");
	#endif
		if(f==NULL)
		{
			return false;
		}

		fclose(f);
		return true;
	}

	//-----------------------------------------------------------------------

	void RemoveFile(const tWString& asFilePath)
	{
	#ifdef _WIN32
		_wremove(asFilePath.c_str());
	#else
		remove(cString::To8Char(asFilePath).c_str());
	#endif
	}

	//-----------------------------------------------------------------------

	bool CloneFile(const tWString& asSrcFileName,const tWString& asDestFileName,
		bool abFailIfExists)
	{
	#ifdef _WIN32
		return CopyFile(asSrcFileName.c_str(),asDestFileName.c_str(),abFailIfExists)==TRUE;
	#else
		if (abFailIfExists && FileExists(asDestFileName)) {
			return true;
		}
		std::ifstream IN (cString::To8Char(asSrcFileName).c_str(), std::ios::binary);
		std::ofstream OUT (cString::To8Char(asDestFileName).c_str(), std::ios::binary);
		OUT << IN.rdbuf();
		OUT.flush();
		return true;
	#endif
	}

	//-----------------------------------------------------------------------

	bool CreateFolder(const tWString& asPath)
	{
	#ifdef _WIN32
		return CreateDirectory(asPath.c_str(),NULL)==TRUE;
	#else
		return mkdir(cString::To8Char(asPath).c_str(),0755)==0;
	#endif
	}

	bool FolderExists(const tWString& asPath)
	{
	#ifdef _WIN32
		return GetFileAttributes(asPath.c_str())==FILE_ATTRIBUTE_DIRECTORY;
	#else
		struct stat statbuf;
		return (stat(cString::To8Char(asPath).c_str(), &statbuf) != -1);
	#endif
	}

	bool IsFileLink(const tWString& asPath)
	{
	// Symbolic Links Not Supported under Windows
	#ifndef _WIN32
		struct stat statbuf;
		if (lstat(cString::To8Char(asPath).c_str(), &statbuf) == 0) {
			return statbuf.st_mode == S_IFLNK;
		} else {
			return false;
		}
	#else
			return false;
	#endif
	}

	bool LinkFile(const tWString& asPointsTo, const tWString& asLink)
	{
	// Symbolic Links Not Supported under Windows
	#ifndef _WIN32
		tWString cmd = _W("ln -s \"") + asPointsTo + _W("\" \"") + asLink + _W("\"");
		return (system(cString::To8Char(cmd).c_str()) == 0);
	#else
		return false;
	#endif
	}

	bool RenameFile(const tWString& asFrom, const tWString& asTo)
	{
	#ifdef _WIN32
		return false;
	#else
		return (rename(cString::To8Char(asFrom).c_str(), cString::To8Char(asTo).c_str()) == 0);
	#endif
	}

	//-----------------------------------------------------------------------

	cDate FileModifiedDate(const tWString& asFilePath)
	{
		struct tm* pClock;
	#ifdef _WIN32
		struct _stat attrib;
		_wstat(asFilePath.c_str(), &attrib);
	#else
		struct stat attrib;
		stat(cString::To8Char(asFilePath).c_str(), &attrib);
	#endif

		pClock = gmtime(&(attrib.st_mtime));	// Get the last modified time and put it into the time structure

		cDate date = DateFromGMTIme(pClock);

		return date;
	}

	//-----------------------------------------------------------------------

	cDate FileCreationDate(const tWString& asFilePath)
	{
		struct tm* pClock;
	#ifdef _WIN32
		struct _stat attrib;
		_wstat(asFilePath.c_str(), &attrib);
	#else
		struct stat attrib;
		stat(cString::To8Char(asFilePath).c_str(), &attrib);
	#endif

		pClock = gmtime(&(attrib.st_ctime));	// Get the last modified time and put it into the time structure

		cDate date = DateFromGMTIme(pClock);

		return date;
	}

	//-----------------------------------------------------------------------

	void SetWindowCaption(const tString &asName)
	{
		// TODO: this requires a ref to window, but this is only called from Init when gpInit is not yet set, suggest move to lowlevelgraphics
	}

	//-----------------------------------------------------------------------

	bool HasWindowFocus(const tWString &asWindowCaption)
	{
		#ifdef _WIN32
			HWND pWindowHandle = FindWindow(NULL, asWindowCaption.c_str());
			return (pWindowHandle == GetFocus());
		#else
		return true;
		#endif
	}

	//-----------------------------------------------------------------------

	unsigned long GetApplicationTime()
	{
		return SDL_GetTicks();
	}

	//-----------------------------------------------------------------------

	unsigned long cLowLevelSystemSDL::GetTime()
	{
		return SDL_GetTicks();
	}

	//-----------------------------------------------------------------------

	cDate cLowLevelSystemSDL::GetDate()
	{
		time_t lTime;
		time(&lTime);

		struct tm* pClock;
		pClock = localtime(&lTime);

		return DateFromGMTIme(pClock);
	}

	//-----------------------------------------------------------------------

	void cLowLevelSystemSDL::Sleep ( const unsigned int alMillisecs )
	{
		SDL_Delay ( alMillisecs );
	}

	//-----------------------------------------------------------------------
	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// STATIC PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

}
