/**@file
 * @brief      CFtpServer - Source code
 *
 * @author     Mail :: thebrowser@gmail.com
 *
 * @copyright  Copyright (C) 2007 Julien Poumailloux
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. If you like this software, a fee would be appreciated, particularly if
 *    you use this software in a commercial product, but is not required.
 */


#include "CFtpServer.h"
#include "CFtpServerGlobal.h"

/**
 * Portable function that sleeps for at least the specified interval.
 *
 * @param  msec  the delay to sleep in Milliseconds.
 */
void sleep_msec(int msec)
{
	#ifndef WIN32
		struct ::timespec tval, trem;
	
		tval.tv_sec = msec / 1000;
		tval.tv_nsec = (msec % 1000) * 1000000;
	
		while(::nanosleep(&tval, &trem) == -1 && errno == EINTR)
		{
			tval.tv_sec = trem.tv_sec;
			tval.tv_nsec = trem.tv_nsec;
		}
	#else
		Sleep( msec );
	#endif
}

//////////////////////////////////////////////////////////////////////
// CFtpServer CLASS
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////
// [CONSTRUCTOR / DESTRUCTOR]
////////////////////////////////////////

CFtpServer::CFtpServer(void)
{
	pFirstUser = pLastUser = NULL;
	pFirstClient = pLastClient = NULL;

	usListeningPort = 21; // By default, the FTP control port is 21
	bIsListening = bIsAccepting = false;
	uiNumberOfUser = uiNumberOfClient = 0;

	DataPortRange.usStart = 100; // TCP Ports [100;999].
	DataPortRange.usLen = 900;

	ulNoTransferTimeout = ulNoLoginTimeout = 0; // No timeout.
	uiCheckPassDelay = 0; // No pass delay.
	uiMaxPasswordTries = 3; // 3 pass tries before the client gets kicked.

	uiTransferBufferSize = 32 * 1024;
	uiTransferSocketBufferSize = 64 * 1024;

	ListeningSock = INVALID_SOCKET;

	srand((unsigned) time(NULL));

#ifndef WIN32
	AcceptingThreadID = 0;
	pthread_attr_init( &m_pattrServer );
	pthread_attr_setstacksize( &m_pattrServer, 5*1024 );
	pthread_attr_init( &this->m_pattrClient );
	pthread_attr_setstacksize( &m_pattrClient, 10*1024 );
	pthread_attr_init( &this->m_pattrTransfer );
	pthread_attr_setstacksize( &m_pattrTransfer, 5*1024 );
#else
	hAcceptingThread = INVALID_HANDLE_VALUE;
	uAcceptingThreadID = 0;
#endif

#ifdef CFTPSERVER_ENABLE_EVENTS
	_OnUserEventCb = NULL;
	_OnClientEventCb = NULL;
	_OnServerEventCb = NULL;
#endif

#ifdef CFTPSERVER_ENABLE_ZLIB
	bEnableZlib = false;
#endif
	bEnableFXP = false;

	FtpServerLock.Initialize();
	UserListLock.Initialize();
	ClientListLock.Initialize();
}

CFtpServer::~CFtpServer(void)
{
	if( IsListening() )
		StopListening();
	CUserEntry *pUser = pFirstUser, *pNextUser;
	while( pUser ) {
		pNextUser = pUser->pNextUser;
		DeleteUser( pUser );
		pUser = pNextUser;
	}
	while( uiNumberOfUser ) {
		sleep_msec( 100 );
	}

	UserListLock.Destroy();
	ClientListLock.Destroy();
	FtpServerLock.Destroy();
}

////////////////////////////////////////
// CONFIG
////////////////////////////////////////

bool CFtpServer::SetDataPortRange( unsigned short int usStart, unsigned short int usLen )
{
	if( usLen != 0 && usStart > 0 && (usStart + usLen) <= 65535 ) {
		FtpServerLock.Enter();
		{
			DataPortRange.usStart = usStart;
			DataPortRange.usLen = usLen;
		}
		FtpServerLock.Leave();
		return true;
	}
	return false;
}

bool CFtpServer::GetDataPortRange( unsigned short int *usStart, unsigned short int *usLen )
{
	if( usStart && usLen ) {
		FtpServerLock.Enter();
		{
			*usStart = this->DataPortRange.usStart;
			*usLen = this->DataPortRange.usLen;
		}
		FtpServerLock.Leave();
		return true;
	}
	return false;
}

////////////////////////////////////////
// [START / STOP] THE SERVER
////////////////////////////////////////

bool CFtpServer::StartListening( unsigned long ulAddr, unsigned short int usPort )
{
	if( ulAddr == INADDR_NONE || usPort == 0 )
		return false;
	FtpServerLock.Enter();
	{
		while( IsListening() ) {
			FtpServerLock.Leave();
			StopListening();
			FtpServerLock.Enter();
		}
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = ulAddr;
		sin.sin_port = htons( usPort );
		#ifdef USE_BSDSOCKETS
			sin.sin_len = sizeof( sin );
		#endif
		#ifndef WIN32
			int on = 1;
		#endif
		this->ListeningSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if( ListeningSock != INVALID_SOCKET
			#ifndef WIN32 // On win32, SO_REUSEADDR allow 2 sockets to listen on the same TCP-Port.
				&& setsockopt( ListeningSock, SOL_SOCKET, SO_REUSEADDR,(char *) &on, sizeof( on ) ) != SOCKET_ERROR
			#endif
			&& bind( ListeningSock, (struct sockaddr *) &sin, sizeof( struct sockaddr_in ) ) != SOCKET_ERROR
			&& listen( ListeningSock, 0) != SOCKET_ERROR )
		{
			usListeningPort = usPort;
			bIsListening = true;
			OnServerEventCb( START_LISTENING );
		} else
			if( ListeningSock != INVALID_SOCKET) CloseSocket( ListeningSock );
	}
	FtpServerLock.Leave();
	return bIsListening;
}

bool CFtpServer::StopListening( void )
{
	FtpServerLock.Enter();
	{
		if( IsListening() ) {
			SOCKET tmpSock = ListeningSock;
			ListeningSock = INVALID_SOCKET;
			CloseSocket( tmpSock );
			if( IsAccepting() ) {
				#ifdef WIN32
					WaitForSingleObject( hAcceptingThread, INFINITE );
					CloseHandle( hAcceptingThread );
				#else
					if( !pthread_cancel( AcceptingThreadID ) )
						pthread_join( AcceptingThreadID, NULL );
				#endif
			}
			bIsListening = false;
			FtpServerLock.Leave();
			OnServerEventCb( STOP_LISTENING );
			FtpServerLock.Enter();
		}
	}
	FtpServerLock.Leave();
	return true;
}

bool CFtpServer::StartAccepting( void )
{
	FtpServerLock.Enter();
	{
		if( !IsAccepting() && IsListening() ) {
#ifdef WIN32
			hAcceptingThread = (HANDLE) _beginthreadex( NULL, 0,
				StartAcceptingEx, this, 0, &uAcceptingThreadID );
			if( hAcceptingThread != 0 ) {
#else
			if( pthread_create( &AcceptingThreadID, &m_pattrServer,
				StartAcceptingEx, this ) == 0 ) {
#endif
				OnServerEventCb( START_ACCEPTING );
				bIsAccepting = true;
			} else {
				OnServerEventCb( THREAD_ERROR );
			}
		}
	}
	FtpServerLock.Leave();	
	return bIsAccepting;
}

#ifdef WIN32
	unsigned CFtpServer::StartAcceptingEx( void *pvParam )
#else
	void* CFtpServer::StartAcceptingEx( void *pvParam )
#endif
{
	class CFtpServer *pFtpServer = ( class CFtpServer* ) pvParam;

	fd_set fdRead;
	SOCKET TempSock; struct sockaddr_in Sin;
	socklen_t Sin_len = sizeof(struct sockaddr_in);
	SOCKET ListeningSock = pFtpServer->ListeningSock;

	for( ;; ) {

		FD_ZERO( &fdRead );
		FD_SET( ListeningSock, &fdRead );
		if( select( ListeningSock + 1, &fdRead, NULL, NULL, NULL ) == SOCKET_ERROR )
			break;

		TempSock = accept( ListeningSock, (struct sockaddr *) &Sin, &Sin_len);
		if( TempSock != INVALID_SOCKET ) {
			CFtpServer::CClientEntry *pClient = pFtpServer->AddClient( TempSock, &Sin );
			if( pClient != NULL ) {
#ifdef WIN32
				pClient->hClientThread = (HANDLE) _beginthreadex( NULL, 0,
					pClient->Shell, pClient, 0, &pClient->uClientThreadID );
				if( pClient->hClientThread == 0 ) {
#else
				if( pthread_create( &pClient->ClientThreadID, &pFtpServer->m_pattrClient,
					pClient->Shell, pClient ) != 0 ) {
#endif
					delete pClient;
					pFtpServer->OnServerEventCb( THREAD_ERROR );
				}
			}
		} else
			break;
	}

	pFtpServer->bIsAccepting = false;
	pFtpServer->bIsListening = false;
	pFtpServer->OnServerEventCb( STOP_ACCEPTING );
	pFtpServer->OnServerEventCb( STOP_LISTENING );

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

////////////////////////////////////////
// FILE
////////////////////////////////////////

char* CFtpServer::CClientEntry::BuildPath( char* pszAskedPath, char **pszVirtualPath /* = NULL */ )
{
	if( !pUser )
		return NULL; // Client is not logged-in.

	char *pszBuffer = new char[ MAX_PATH + 4 ];
	if( pszBuffer ) {

		char *pszVirtualP = BuildVirtualPath( pszAskedPath );
		if( pszVirtualP ) {

			if( snprintf( pszBuffer, MAX_PATH + 4, "%s/%s",
				pUser->GetStartDirectory(), pszVirtualP ) > 0 )
			{
				SimplifyPath( pszBuffer );
				if( strlen( pszBuffer ) <= MAX_PATH ) {
					if( pszVirtualPath ) {
						*pszVirtualPath = pszVirtualP;
					} else
						delete [] pszVirtualP;
					return pszBuffer;
				}
			}

		}
		delete [] pszBuffer;

	} else
		pFtpServer->OnServerEventCb( MEM_ERROR );

	return NULL;
}

char* CFtpServer::CClientEntry::BuildVirtualPath( char* pszAskedPath )
{
	char *pszBuffer = new char[ MAX_PATH + 4 ];
	if( pszBuffer ) {

		bool bErr = false;
		if( pszAskedPath ) {
			if( pszAskedPath[ 0 ] == '/' ) {
				CFtpServer::SimplifyPath( pszAskedPath );
				if( strlen( pszAskedPath ) <= MAX_PATH ) {
					strcpy( pszBuffer, pszAskedPath );
				} else
					bErr = true;
			} else {
				if( snprintf( pszBuffer, MAX_PATH + 4, "%s/%s",
						GetWorkingDirectory(), pszAskedPath ) > 0 )
				{
					SimplifyPath( pszBuffer );
					if( strlen( pszBuffer ) > MAX_PATH )
						bErr = true;
				}
			}
		} else
			strcpy( pszBuffer, GetWorkingDirectory() );

		if( bErr == false && strlen( pszBuffer ) <= MAX_PATH ) {
			return pszBuffer;
		} else
			delete [] pszBuffer;
		
	} else
		pFtpServer->OnServerEventCb( MEM_ERROR );

	return NULL;
}

bool CFtpServer::SimplifyPath( char *pszPath )
{
	if( !pszPath || *pszPath == 0 )
		return false;

	char *a;
	for( a = pszPath; *a != '\0'; ++a ) // Replace all '\' by a '/'
		if( *a == '\\' ) *a = '/';
	while( ( a = strstr( pszPath, "//") ) != NULL ) // Replace all "//" by "/"
		memmove( a, a + 1 , strlen( a ) );
	while( ( a = strstr( pszPath, "/./" ) ) != NULL ) // Replace all "/./" by "/"
		memmove( a, a + 2, strlen( a + 1 ) );
	if( !strncmp( pszPath, "./", 2 ) )
		memmove( pszPath, pszPath + 1, strlen( pszPath ) );
	if( !strncmp( pszPath, "../", 3) )
		memmove( pszPath, pszPath + 2, strlen( pszPath + 1 ) );
	if( ( a = strstr( pszPath, "/." ) ) && a[ 2 ] == '\0' )
		a[ 1 ] = '\0';

	a = pszPath; // Remove the ending '.' in case the path ends with "/."
	while( ( a = strstr( a, "/." ) ) != NULL ) { 
		if( a[ 2 ] == '\0' ) {
			a[ 1 ] = '\0';
		} else
			a++;
	}

	a = pszPath; char *b = NULL;
	while( a && (a = strstr( a, "/..")) ) {
		if( a[3] == '/' || a[3] == '\0' ) {
			if( a == pszPath ) {
				if( a[3] == '/' ) {
					memmove( pszPath, pszPath + 3, strlen( pszPath + 2 ) );
				} else {
					pszPath[ 1 ] = '\0';
					break;
				}
			} else {
				b = a;
				while( b != pszPath ) {
					--b;
					if( *b == '/' ) {
						if( b == pszPath
							|| ( b == ( pszPath + 2 ) && isalpha( pszPath[ 0 ] ) && pszPath[ 1 ] == ':' ) // C:
						) {
							if( a[ 3 ] == '/' ) { // e.g. '/foo/../' or 'C:/lol/../'
								memmove( b, a + 3, strlen( a + 3 ) + 1 ); // +1 for the \0
							} else // e.g. '/foo/..' 'C:/foo/..'
								b[ 1 ] = '\0';
						} else
							memmove( b, a + 3, strlen( a + 2 ) );
						a = strstr( pszPath, "/.." );
						break;
					} else if( b == pszPath ) {
						if( a[ 3 ] == '/' ) { // e.g. C:/../
							memmove( a, a + 3, strlen( a + 3 ) + 1 ); // +1 for the \0
						} else // e.g. C:/..
							a[ 1 ] = '\0';
						a = strstr( pszPath, "/.." );
					}
				}
			}
		} else
			++a;
	}

	// Remove the ending '/'
	int iPathLen = strlen( pszPath );
	if( isalpha( pszPath[0] ) && pszPath[1] == ':' && pszPath[2] == '/' ) {
		if( iPathLen > 3 && pszPath[ iPathLen -1 ] == '/' ) { // "C:/some/path/"
			pszPath[ iPathLen - 1 ] = '\0';
			--iPathLen;
		}
	} else {
		if( pszPath[ 0 ] == '/' ) {
			if( iPathLen > 1 && pszPath[ iPathLen - 1 ] == '/' ) {
				pszPath[ iPathLen - 1 ] = '\0'; // "/some/path/"
				--iPathLen;
			}
		} else if( pszPath[ iPathLen - 1 ] == '/' ) {
			pszPath[ iPathLen - 1 ] = '\0'; // "some/path/"
			--iPathLen;
		}
	}

	if( *pszPath == 0 || !strcmp( pszPath, ".." ) || !strcmp( pszPath, "." ) )
		strcpy( pszPath, "/" );

	return true;
}

////////////////////////////////////////
// USER
////////////////////////////////////////

CFtpServer::CUserEntry *CFtpServer::AddUser( const char *pszLogin, const char *pszPass, const char *pszStartDir )
{
	if( pszLogin && pszStartDir 
		&& strlen( pszLogin ) <= MaxLoginLen
		&& strlen( pszStartDir ) <= MaxRootPathLen
		&& ( !pszPass || strlen( pszPass ) <= MaxPasswordLen )	)
	{
		bool bErr = false;
		UserListLock.Enter();
		{
			if( SearchUserFromLogin( pszLogin ) != NULL )
				bErr = true; // A user with that name already exists.
		}
		UserListLock.Leave();
		if( bErr )
			return NULL;

		char *pszSDEx = strdup( pszStartDir );
		if( !pszSDEx ) return NULL;
		SimplifyPath( pszSDEx );
		struct stat st; // Verify that the StartPath exist, and is a directory
		if( stat( pszSDEx, &st ) != 0 || !S_ISDIR( st.st_mode ) ) {
			free( pszSDEx );
			return NULL;
		}

		CFtpServer::CUserEntry *pUser = new CUserEntry;

		if( pUser ) {

			strcpy( pUser->szLogin, pszLogin );
			strcpy( pUser->szStartDirectory, pszSDEx ); free( pszSDEx );
			if( pszPass ) strcpy( pUser->szPassword, pszPass );

			UserListLock.Enter();
			{
				if( pLastUser ) {
					pUser->pPrevUser = pLastUser;
					pLastUser->pNextUser = pUser;
					pLastUser = pUser;
				} else pFirstUser = pLastUser = pUser;
			}
			UserListLock.Leave();
			
			pUser->bIsEnabled = true;
			++uiNumberOfUser;
			
			OnUserEventCb( NEW_USER, pUser, NULL );
	
			return pUser;

		} else
			OnServerEventCb( MEM_ERROR );
	}
	return NULL;
}

bool CFtpServer::DeleteUser( CFtpServer::CUserEntry *pUser )
{
	if( pUser != NULL ) {

		UserListLock.Enter();
		{
			
			pUser->bIsEnabled = false;

			if( pUser->pPrevUser ) pUser->pPrevUser->pNextUser = pUser->pNextUser;
			if( pUser->pNextUser ) pUser->pNextUser->pPrevUser = pUser->pPrevUser;
			if( pFirstUser == pUser ) pFirstUser = pUser->pNextUser;
			if( pLastUser == pUser ) pLastUser = pUser->pPrevUser;

			ClientListLock.Enter();
			{
				CFtpServer::CClientEntry *pClient, *pNextClient;
				pClient = pFirstClient;
				while( pClient ) {
					pNextClient = pClient->pNextClient;
					if( pClient->pUser == pUser ) {
						if( pClient->bIsLogged ) {
							pClient->InitDelete();
						} else
							pClient->pUser = NULL;
					}
					pClient = pNextClient;
				}
				if( pUser->GetNumberOfClient() ) {
					pUser->bDelete = true;
				} else
					delete pUser;
			}
			ClientListLock.Leave();
			--uiNumberOfUser;
		}
		UserListLock.Leave();
		return true;
	}
	return false;
}

////////////////////////////////////////
// CLIENT
////////////////////////////////////////

CFtpServer::CClientEntry *CFtpServer::AddClient( SOCKET Sock, struct sockaddr_in *Sin )
{
	if( Sock != INVALID_SOCKET && &Sin ) {

		CFtpServer::CClientEntry *pClient = new CFtpServer::CClientEntry;

		if( pClient ) {
		
			pClient->CtrlSock = Sock;
			pClient->DataSock = INVALID_SOCKET;

			struct sockaddr ServerSin;
			socklen_t Server_Sin_Len = sizeof( ServerSin );
			getsockname( Sock, &ServerSin, &Server_Sin_Len );
 			pClient->ulServerIP = (((struct sockaddr_in *) &(ServerSin))->sin_addr).s_addr;
			pClient->ulClientIP = (((struct sockaddr_in *) (Sin))->sin_addr).s_addr;

			pClient->pFtpServer = this;

			strcpy( pClient->szWorkingDir, "/" );

			ClientListLock.Enter();
			{
				if( pLastClient ) {
					pClient->pPrevClient = pLastClient;
					pLastClient->pNextClient = pClient;
					pLastClient = pClient;
				} else pFirstClient = pLastClient = pClient;
			}
			ClientListLock.Leave();

			pClient->ClientLock.Initialize();
			++uiNumberOfClient;

			OnClientEventCb( NEW_CLIENT, pClient );

			return pClient;
		} else
			OnServerEventCb( MEM_ERROR );
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// CFtpServer::CUserEntry CLASS
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////
// CONSTRUCTOR
////////////////////////////////////////

CFtpServer::CUserEntry::CUserEntry()
{
	bDelete = false;
	bIsEnabled = false;
	ucPrivileges = 0;
	#ifdef CFTPSERVER_ENABLE_EXTRACMD
		ucExtraCommand = 0;
	#endif
	pFtpServer = NULL;
	pPrevUser = pNextUser = NULL;
	uiNumberOfClient = uiMaxNumberOfClient = 0;
	*szLogin = *szPassword = *szStartDirectory = '\0';
}

////////////////////////////////////////
// CONFIG
////////////////////////////////////////

#ifdef CFTPSERVER_ENABLE_EXTRACMD
bool CFtpServer::CUserEntry::SetExtraCommand( unsigned char ucExtraCmd )
{
	if( ( ucExtraCmd & ~( ExtraCmd_EXEC ) ) == 0 ) {
		ucExtraCommand = ucExtraCmd;
		return true;
	}
	return false;
}
#endif

bool CFtpServer::CUserEntry::SetPrivileges( unsigned char ucPriv )
{
	if( (ucPriv & ~(READFILE|WRITEFILE|DELETEFILE|LIST|CREATEDIR|DELETEDIR)) == 0) {
		ucPrivileges = ucPriv;
		return true;
	}
	return false;
}

////////////////////////////////////////
// USER LIST
////////////////////////////////////////

CFtpServer::CUserEntry *CFtpServer::SearchUserFromLogin( const char *pszLogin )
{
	if( pszLogin ) {
		CFtpServer::CUserEntry *pSearchUser = NULL;
		pSearchUser = pFirstUser;
		while( pSearchUser ) {
			if( pSearchUser->szLogin && !strcmp( pszLogin, pSearchUser->szLogin ) ) {
				break;
			}
			pSearchUser = pSearchUser->pNextUser;
		}
		return pSearchUser;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// CFtpServer::CClientEntry CLASS
//////////////////////////////////////////////////////////////////////

////////////////////////////////////////
// CONSTRUCTOR / DESTRUCTOR
////////////////////////////////////////

CFtpServer::CClientEntry::CClientEntry()
{
	pUser = NULL;
	iZlibLevel = 0;
	pFtpServer = NULL;
	bIsLogged = false;
	eStatus = WAITING;
	eDataType = ASCII;
	eDataMode = STREAM;
	nPasswordTries = 0;
	eDataConnection = NONE;
	bIsCtrlCanalOpen = false;
	ulDataIp = usDataPort = 0;
	ulServerIP = ulClientIP = 0;
	pszCmdArg = psNextCmd = NULL;
	pPrevClient = pNextClient = NULL;
	CtrlSock = DataSock = INVALID_SOCKET;
	*szWorkingDir = *szRenameFromPath = '\0';
	iCmdLen = iCmdRecvdLen = nRemainingCharToParse = 0;

	CurrentTransfer.RestartAt = 0;
#ifdef WIN32
	hClientThread = CurrentTransfer.hTransferThread = INVALID_HANDLE_VALUE;
	uClientThreadID = CurrentTransfer.uTransferThreadID = 0;
#else
	ClientThreadID = CurrentTransfer.TransferThreadID = 0;
#endif

#ifdef CFTPSERVER_ENABLE_ZLIB
	iZlibLevel = 6; // Default Zlib compression level.
#endif

}

CFtpServer::CClientEntry::~CClientEntry()
{
	pFtpServer->OnClientEventCb( DELETE_CLIENT, this );
	if( bIsLogged && pUser ) {
		--pUser->uiNumberOfClient;
		if( pUser->uiNumberOfClient == 0 && pUser->bDelete ) {
			delete pUser;
		}
	}
	if( pPrevClient ) pPrevClient->pNextClient = pNextClient;
	if( pNextClient ) pNextClient->pPrevClient = pPrevClient;
	if( pFtpServer ) {
		--pFtpServer->uiNumberOfClient;
		if( pFtpServer->pFirstClient == this ) pFtpServer->pFirstClient = pNextClient;
		if( pFtpServer->pLastClient == this ) pFtpServer->pLastClient = pPrevClient;
	}
	ClientLock.Destroy();
}

bool CFtpServer::CClientEntry::InitDelete()
{
	ClientLock.Enter();
	{
		SOCKET tmpSock;
		if( CtrlSock != INVALID_SOCKET ) {
			tmpSock = CtrlSock;
			CtrlSock = INVALID_SOCKET;
			CloseSocket( tmpSock );
		}
		if( DataSock != INVALID_SOCKET ) {
			tmpSock = DataSock;
			DataSock = INVALID_SOCKET;
			CloseSocket( tmpSock );
		}
	}
	ClientLock.Leave();
	return true;
}

////////////////////////////////////////
// PRIVILEGES
////////////////////////////////////////

bool CFtpServer::CClientEntry::CheckPrivileges( unsigned char ucPriv ) const
{ 
	return ( bIsLogged && pUser && (ucPriv & pUser->ucPrivileges ) == ucPriv ) ? true : false;
}

////////////////////////////////////////
// SHELL
////////////////////////////////////////

#ifdef WIN32
	unsigned CFtpServer::CClientEntry::Shell( void *pvParam )
#else
	void *CFtpServer::CClientEntry::Shell( void *pvParam )
#endif
{
	CFtpServer::CClientEntry *pClient = (CFtpServer::CClientEntry*) pvParam;
	CFtpServer *pFtpServer = pClient->pFtpServer;
    

    pClient->SendReply( "220 Welcome to the Modizer FTP Server." );

	pClient->bIsCtrlCanalOpen = true;
	pClient->eStatus = WAITING;

	struct stat st;
	char *pszPath = NULL;

	int nCmd;
	char *pszCmdArg = NULL;

	pClient->ResetTimeout();
        
	for( ;; ) { // Main Loop

		if( pszPath ) {
			delete [] pszPath;
			pszPath = NULL;
		}

		if( pClient->ReceiveLine() == false )
			break;
		nCmd = pClient->ParseLine();
		pszCmdArg = pClient->pszCmdArg;

		if( nCmd == CMD_NONE ) {

			pClient->SendReply( "500 Command not understood." );
			continue;

		} else if( nCmd == CMD_QUIT ) {

			pClient->SendReply( "221 Goodbye." );
			pFtpServer->OnClientEventCb( CLIENT_DISCONNECT, pClient );
			break;

		} else if( nCmd == CMD_USER ) {

			if( pClient->bIsLogged == true )
				pClient->LogOut();
			if( !pszCmdArg ) {
				pClient->SendReply( "501 Invalid number of arguments." );
			} else {
				pFtpServer->UserListLock.Enter();
				{
					pClient->pUser = pFtpServer->SearchUserFromLogin( pszCmdArg );
					if( pClient->pUser && pClient->pUser->bIsEnabled && !*pClient->pUser->szPassword ) {
						pClient->LogIn();
					} else
						pClient->SendReply( "331 Password required for this user." );
				}
				pFtpServer->UserListLock.Leave();
			}
			continue;

		} else if( nCmd == CMD_PASS ) {

			++pClient->nPasswordTries;
			if( pClient->bIsLogged == true ) {
				pClient->SendReply( "230 User Logged In." );
			} else {
				if( pFtpServer->GetCheckPassDelay() ) sleep_msec( pFtpServer->GetCheckPassDelay() );
				pFtpServer->UserListLock.Enter();
				{
					if( pClient->pUser && pClient->pUser->bIsEnabled ) { // User is valid & enabled
						if( ( pszCmdArg && !strcmp( pszCmdArg, pClient->pUser->szPassword ) )
							|| !*pClient->pUser->szPassword ) // Password is valid.
						{
							if( pClient->pUser->uiMaxNumberOfClient == 0 //  0 = Unlimited
								||  pClient->pUser->uiNumberOfClient < pClient->pUser->uiMaxNumberOfClient )
							{
								pClient->LogIn();
							} else
								pClient->SendReply( "421 Too many users logged in for this account." );
						} else if( !pszCmdArg )
							pClient->SendReply( "501 Invalid number of arguments." );
					}
				}
				pFtpServer->UserListLock.Leave();
				if( pClient->bIsLogged ) {
					pFtpServer->OnClientEventCb( CLIENT_AUTH, pClient );
					continue;
				}
			}
			if( pFtpServer->GetMaxPasswordTries() && pClient->nPasswordTries >= pFtpServer->GetMaxPasswordTries() ) {
				pFtpServer->OnClientEventCb( TOO_MANY_PASS_TRIES, pClient );
				break;
			}
			pClient->SendReply( "530 Login or password incorrect!" );
			continue;

		} else if( pClient->bIsLogged == false ) {

			pClient->SendReply( "530 Please login with USER and PASS." );
			continue;

		} else if( nCmd == CMD_NOOP || nCmd == CMD_ALLO ) {

			pClient->SendReply( "200 NOOP Command Successful." );
			continue;

		#ifdef CFTPSERVER_ENABLE_EXTRACMD
		} else if( nCmd == CMD_SITE ) {

			char *pszSiteArg = pszCmdArg;
			if( pszCmdArg ) {
				unsigned char ucExtraCmd = pClient->pUser->GetExtraCommand();
				if( !strncasecmp( pszCmdArg, "EXEC ", 5 ) ) {
					if( (ucExtraCmd & ExtraCmd_EXEC) == ExtraCmd_EXEC ) {
						if( strlen( pszSiteArg ) >= 5 )
							system( pszSiteArg + 5 );
							pClient->SendReply( "200 SITE EXEC Successful." );
						continue;
					} else pClient->SendReply( "550 Permission denied." );
				}
			}
			pClient->SendReply( "501 Syntax error in arguments." );
			continue;
		#endif

		} else if( nCmd == CMD_HELP ) {

			pClient->SendReply( "500 No Help Available." );
			continue;

		} else if( nCmd == CMD_SYST ) {

			pClient->SendReply( "215 UNIX Type: L8" );
			continue;

		} else if( nCmd == CMD_CLNT ) {

			if( pszCmdArg ) {
				pFtpServer->OnClientEventCb( CLIENT_SOFTWARE, pClient, pszCmdArg );
				pClient->SendReply( "200 Ok." );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_STRU ) {

			if( pszCmdArg ) {
				if( !strcmp( pszCmdArg, "F" ) ) {
					pClient->SendReply( "200 STRU Command Successful." );
				} else
					pClient->SendReply( "504 STRU failled. Parametre not implemented" );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_OPTS ) {

			if( pszCmdArg ) {
#ifdef CFTPSERVER_ENABLE_ZLIB
				if( !strncasecmp( pszCmdArg, "mode z level ", 13 ) ) {
					int iLevel = atoi( pszCmdArg + 13 );
					if( iLevel > 0 && iLevel <= 9 ) {
						pClient->iZlibLevel = iLevel;
						pClient->SendReply( "200 MODE Z LEVEL successfully set." );
					} else
						pClient->SendReply( "501 Invalid MODE Z LEVEL value." );
				} else
#endif
					pClient->SendReply( "501 Option not understood." );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_MODE ) {

			if( pszCmdArg ) {
				if( !strcmp( pszCmdArg, "S" ) ) { // Stream
					pClient->eDataMode = STREAM;
					pClient->SendReply( "200 MODE set to S." );
#ifdef CFTPSERVER_ENABLE_ZLIB
				} else if( !strcmp( pszCmdArg, "Z" ) ) {
					if( pFtpServer->IsModeZEnabled() ) {
						pClient->eDataMode = ZLIB;
						pClient->SendReply( "200 MODE set to Z." );
					} else
						pClient->SendReply( "504 MODE Z disabled." );
#else
				} else if( !strcmp( pszCmdArg, "Z" ) ) {
					pClient->SendReply( "502 MODE Z non-implemented." );
#endif
				} else
					pClient->SendReply2( "504 \"%s\": Unsupported transfer MODE.", pszCmdArg );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_TYPE ) {

			if( pszCmdArg ) {
				if( pszCmdArg[0] == 'A' ) {
					pClient->eDataType = ASCII; // Infact, ASCII mode isn't supported..
					pClient->SendReply( "200 ASCII transfer mode active." );
				} else if( pszCmdArg[0] == 'I' ) {
					pClient->eDataType = BINARY;
					pClient->SendReply( "200 Binary transfer mode active." );
				} else 
					pClient->SendReply( "550 Error - unknown binary mode." );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_FEAT ) {

#ifdef CFTPSERVER_ENABLE_ZLIB
			char szFeat[ 73 ] = "211-Features:\r\n"
				" CLNT\r\n"
				" MDTM\r\n";
			if( pFtpServer->IsModeZEnabled() )
				strcat( szFeat, " MODE Z\r\n" );
			strcat( szFeat, " REST STREAM\r\n"
				" SIZE\r\n"
				"211 End" );
			pClient->SendReply( szFeat );
#else
			pClient->SendReply( "211-Features:\r\n"
				" CLNT\r\n"
				" MDTM\r\n"
				" REST STREAM\r\n"
				" SIZE\r\n"
				"211 End"  );
#endif

			continue;

		} else if( nCmd == CMD_PORT ) {

			unsigned int iIp1 = 0, iIp2 = 0, iIp3 = 0, iIp4 = 0, iPort1 = 0, iPort2 = 0;
			if( pClient->eStatus == WAITING && pszCmdArg
				&& sscanf( pszCmdArg, "%u,%u,%u,%u,%u,%u", &iIp1, &iIp2, &iIp3, &iIp4, &iPort1, &iPort2 ) == 6
				&& iIp1 <= 255 && iIp2 <= 255 && iIp3 <= 255 && iIp4 <= 255
				&& iPort1 <= 255 && iPort2 <= 255 && (iIp1|iIp2|iIp3|iIp4) != 0
				&& ( iPort1 | iPort2 ) != 0 )
			{
				if( pClient->eDataConnection != NONE )
					pClient->ResetDataConnection();

				char szClientIP[32];
				sprintf( szClientIP, "%u.%u.%u.%u", iIp1, iIp2, iIp3, iIp4 );
				unsigned long ulPortIp = inet_addr( szClientIP );

				if( !pFtpServer->IsFXPEnabled() ||
					( ulPortIp == pClient->ulClientIP || pClient->ulClientIP == inet_addr( "127.0.0.1" ) )
				) {
					pClient->eDataConnection = PORT;
					pClient->ulDataIp = ulPortIp;
					pClient->usDataPort = (unsigned short)(( iPort1 * 256 ) + iPort2);
					pClient->SendReply( "200 PORT command successful." );
				} else
					pClient->SendReply( "501 PORT address does not match originator." );

			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_PASV ) {

			if( pClient->eStatus == WAITING ) {

				if( pClient->eDataConnection != NONE )
					pClient->ResetDataConnection();

				pClient->DataSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
				struct sockaddr_in sin;
				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = pClient->ulServerIP;
				#ifdef USE_BSDSOCKETS
					sin.sin_len = sizeof( sin );
				#endif
				#ifndef WIN32
					int on = 1;
				#endif

				if( pClient->DataSock != INVALID_SOCKET
					#ifndef WIN32
					&& setsockopt( pClient->DataSock, SOL_SOCKET, SO_REUSEADDR,(char *) &on, sizeof( on ) ) != SOCKET_ERROR
					#endif
				) {
					pClient->usDataPort = 0;
					unsigned short int usLen; unsigned short int usStart;
					pFtpServer->GetDataPortRange( &usStart, &usLen );
					unsigned short int usFirstTry = (unsigned short) ( usStart + ( rand() % usLen ) );
					unsigned short usTriedPort = usFirstTry;
					for( ;; ) {
						sin.sin_port = htons( usTriedPort );
						if( !bind( pClient->DataSock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) ) {
							pClient->usDataPort = usTriedPort;
							break;
						}
						--usTriedPort;
						if( usTriedPort < usFirstTry )
							usTriedPort = (unsigned short int)(usStart + usLen);
						if( usTriedPort == usFirstTry ) {
							break;
						}
					}
					if( !pClient->usDataPort ) {
						pClient->SendReply( "451 Internal error - No more data port available." );
						continue;
					}
					if( listen( pClient->DataSock, 1) == SOCKET_ERROR )
						continue;

					unsigned long ulIp = ntohl( pClient->ulServerIP );
					pClient->SendReply2( "227 Entering Passive Mode (%lu,%lu,%lu,%lu,%u,%u)",
						(ulIp >> 24) & 255, (ulIp >> 16) & 255, (ulIp >> 8) & 255, ulIp & 255,
						pClient->usDataPort / 256 , pClient->usDataPort % 256 );

					pClient->eDataConnection = PASV;
				}
			} else
				pClient->SendReply( "425 You're already connected." );
			continue;


		} else if( nCmd == CMD_LIST || nCmd == CMD_NLST || nCmd == CMD_STAT ) {

			if( nCmd == CMD_STAT && !pszCmdArg ) {
				//pClient->SendReply( "211 :: CFtpServer / Browser FTP Server:: thebrowser@gmail.com" );
                pClient->SendReply( "220 You are connected." );
				continue;
			}
			if( !pClient->CheckPrivileges( CFtpServer::LIST ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pClient->eStatus != WAITING ) {
				pClient->SendReply( "425 You're already connected." );
				continue;
			}
			memset( &pClient->CurrentTransfer, 0x0, sizeof( pClient->CurrentTransfer ) );

			char *pszArg = pszCmdArg; // Extract arguments
			if( pszCmdArg ) {
				while( *pszArg == '-' ) {
					while ( ++pszArg && isalnum((unsigned char) *pszArg) ) {
						switch( *pszArg ) {
							case 'a':
								pClient->CurrentTransfer.opt_a = true;
								break;
							case 'd':
								pClient->CurrentTransfer.opt_d = true;
								break;
							case 'F':
								pClient->CurrentTransfer.opt_F = true;
								break;
							case 'l':
								pClient->CurrentTransfer.opt_l = true;
								break;
						}
					}
					while( isspace( (unsigned char) *pszArg ) )
						++pszArg;
				}
			}

			pszPath = pClient->BuildPath( pszArg );
			if( pszPath && stat( pszPath, &st ) == 0 ) {

				memcpy( &pClient->CurrentTransfer.szPath, pszPath, strlen( pszPath ) ); 
				memcpy( &pClient->CurrentTransfer.st, &st, sizeof( st ) );
				if( nCmd == CMD_STAT  ) {
					pClient->SendReply( "213-Status follows:" );
					pClient->CurrentTransfer.SockList = pClient->CtrlSock;
				} else {
					if( pClient->OpenDataConnection( nCmd ) == false )
						continue;
					pClient->CurrentTransfer.SockList = pClient->DataSock;
				}

				pClient->eStatus = LISTING;
				pClient->CurrentTransfer.nCmd = nCmd;
				pClient->CurrentTransfer.pClient = pClient;
#ifdef CFTPSERVER_ENABLE_ZLIB
				pClient->CurrentTransfer.iZlibLevel = pClient->iZlibLevel;
				pClient->CurrentTransfer.eDataMode = pClient->eDataMode;
#endif
				pFtpServer->OnClientEventCb( CLIENT_LIST, pClient, pszPath );

#ifdef WIN32
				pClient->CurrentTransfer.hTransferThread =
					(HANDLE) _beginthreadex( NULL, 0, ListThread, pClient,
					0, &pClient->CurrentTransfer.uTransferThreadID );
				if( pClient->CurrentTransfer.hTransferThread == 0 ) {
#else
				if( pthread_create( &pClient->CurrentTransfer.TransferThreadID,
					&pFtpServer->m_pattrTransfer, ListThread, pClient ) != 0 ) {
#endif
					pClient->ResetDataConnection();
					pFtpServer->OnServerEventCb( THREAD_ERROR );
				}
			}
			continue;

		} else if( nCmd == CMD_CWD || nCmd == CMD_XCWD ) {

			if( pszCmdArg ) {
				char *pszVirtualPath = NULL;
				pszPath = pClient->BuildPath( pszCmdArg, &pszVirtualPath );
				if( pszPath && stat( pszPath, &st ) == 0 && S_ISDIR( st.st_mode ) ) {
					strcpy( pClient->szWorkingDir, pszVirtualPath );
					delete [] pszVirtualPath;
					pClient->SendReply( "250 CWD command successful." );
					pFtpServer->OnClientEventCb( CLIENT_CHANGE_DIR, pClient, pszPath );
				} else
					pClient->SendReply( "550 No such file or directory.") ;
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_MDTM ) {

			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				struct tm *t;
				if( pszPath && !stat( pszPath, &st ) && (t = gmtime((time_t *) &(st.st_mtime))) ) {
					pClient->SendReply2( "213 %04d%02d%02d%02d%02d%02d",
						t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
						t->tm_hour, t->tm_min, t->tm_sec );
				} else
					pClient->SendReply( "550 No such file or directory." );
			} else
				pClient->SendReply( "501 Invalid number of arguments." );
			continue;

		} else if( nCmd == CMD_PWD || nCmd == CMD_XPWD ) {

			pClient->SendReply2( "257 \"%s\" is current directory.", pClient->szWorkingDir);
			continue;

		} else if( nCmd == CMD_CDUP || nCmd == CMD_XCUP ) {

			strncat( pClient->szWorkingDir, "/..", 3 );
			pFtpServer->SimplifyPath( pClient->szWorkingDir );
			pClient->SendReply( "250 CDUP command successful." );
			continue;

		} else if( nCmd == CMD_ABOR ) {

			if( pClient->eStatus != WAITING ) {
				pClient->ResetDataConnection();
				pClient->SendReply( "426 Previous command has been finished abnormally." );
			}
			pClient->SendReply( "226 ABOR command successful." );
			continue;

		} else if( nCmd == CMD_REST ) {

			if( pszCmdArg && pClient->eStatus == WAITING ) {
				#ifdef __USE_FILE_OFFSET64
					pClient->CurrentTransfer.RestartAt = atoll( pszCmdArg );
				#else
					pClient->CurrentTransfer.RestartAt = atoi( pszCmdArg );
				#endif
				pClient->SendReply( "350 REST command successful." );
			} else pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_RETR ) {

			if( !pClient->CheckPrivileges( CFtpServer::READFILE ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg ) {

				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 && S_ISREG( st.st_mode ) ) {

					if( pClient->OpenDataConnection( nCmd ) == false )
						continue;

					pClient->eStatus = DOWNLOADING;
					pClient->CurrentTransfer.pClient = pClient;
					strcpy( pClient->CurrentTransfer.szPath, pszPath );
#ifdef CFTPSERVER_ENABLE_ZLIB
					pClient->CurrentTransfer.iZlibLevel = pClient->iZlibLevel;
					pClient->CurrentTransfer.eDataMode = pClient->eDataMode;
#endif
					pFtpServer->OnClientEventCb( CLIENT_DOWNLOAD, pClient, pszPath );

#ifdef WIN32
					pClient->CurrentTransfer.hTransferThread =
						(HANDLE) _beginthreadex( NULL, 0, RetrieveThread, pClient,
						0, &pClient->CurrentTransfer.uTransferThreadID );
					if( pClient->CurrentTransfer.hTransferThread == 0 ) {
#else
					if( pthread_create( &pClient->CurrentTransfer.TransferThreadID,
						&pFtpServer->m_pattrTransfer, RetrieveThread, pClient ) != 0 ) {
#endif
						pClient->ResetDataConnection();
						pFtpServer->OnServerEventCb( THREAD_ERROR );
					}
				} else
					pClient->SendReply( "550 File not found." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_STOR || nCmd == CMD_APPE || nCmd == CMD_STOU ) {

			if( !pClient->CheckPrivileges( CFtpServer::WRITEFILE ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg || nCmd == CMD_STOU ) {

				char szName[ 32 ];
				if( nCmd == CMD_STOU ) {
					sprintf( szName, "file.%i", rand()%9999999 );
					pszPath = pClient->BuildPath( szName );
					if( !pszPath || !stat( pszPath, &st ) ) {
						delete [] pszPath;
						pszPath = NULL;
					} else
						pClient->SendReply2( "150 FILE: %s", szName );
				} else
					pszPath = pClient->BuildPath( pszCmdArg );

				if( pszPath ) {

					if( !pClient->OpenDataConnection( nCmd ) )
						continue;

					pClient->eStatus = UPLOADING;
					pClient->CurrentTransfer.pClient = pClient;
					if( nCmd == CMD_APPE ) {
						if( stat( pszPath, &st ) == 0 ) {
							pClient->CurrentTransfer.RestartAt = st.st_size;
						} else
							pClient->CurrentTransfer.RestartAt = 0;
					}
					strcpy( pClient->CurrentTransfer.szPath, pszPath );
#ifdef CFTPSERVER_ENABLE_ZLIB
					pClient->CurrentTransfer.iZlibLevel = pClient->iZlibLevel;
					pClient->CurrentTransfer.eDataMode = pClient->eDataMode;
#endif
					pFtpServer->OnClientEventCb( CLIENT_UPLOAD, pClient, pszPath );

#ifdef WIN32
					pClient->CurrentTransfer.hTransferThread =
						(HANDLE) _beginthreadex( NULL, 0, StoreThread, pClient,
						0, &pClient->CurrentTransfer.uTransferThreadID );
					if( pClient->CurrentTransfer.hTransferThread == 0 ) {
#else
					if( pthread_create( &pClient->CurrentTransfer.TransferThreadID,
						&pFtpServer->m_pattrTransfer, StoreThread, pClient ) != 0 ) {
#endif
						pClient->ResetDataConnection();
						pFtpServer->OnServerEventCb( THREAD_ERROR );
					}
				}
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_SIZE ) {
			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 && S_ISREG( st.st_mode ) ) {
					pClient->SendReply2(
						#ifdef __USE_FILE_OFFSET64
							#ifdef WIN32
								"213 %I64i",
							#else
								"213 %llu",
							#endif
						#else
							"213 %li",
						#endif
						st.st_size );
				} else
					pClient->SendReply( "550 No such file." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_DELE ) {

			if( !pClient->CheckPrivileges( CFtpServer::DELETEFILE ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 && S_ISREG( st.st_mode ) ) {
					if( remove( pszPath ) != -1 ) {
						pClient->SendReply( "250 DELE command successful." );
					} else
						pClient->SendReply( "550 Can' t Remove or Access Error." );
				} else
					pClient->SendReply( "550 No such file." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_RNFR ) {

			if( !pClient->CheckPrivileges( CFtpServer::DELETEFILE ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					strcpy( pClient->szRenameFromPath, pszPath );
					pClient->SendReply( "350 File or directory exists, ready for destination name." );
				} else
					pClient->SendReply( "550 No such file or directory." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_RNTO ) {

			if( pszCmdArg ) {
				if( pClient->szRenameFromPath ) {
					pszPath = pClient->BuildPath( pszCmdArg );
					if( pszPath && rename( pClient->szRenameFromPath, pszPath ) == 0 ) {
						pClient->SendReply( "250 Rename successful." );
					} else
						pClient->SendReply( "550 Rename failure." );
					*pClient->szRenameFromPath = 0;
				} else
					pClient->SendReply( "503 Bad sequence of commands" );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_MKD || nCmd == CMD_XMKD ) {

			if( !pClient->CheckPrivileges( CFtpServer::CREATEDIR ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) != 0 ) {
					#ifdef WIN32
						if( _mkdir( pszPath ) == -1 ) {
					#else
						if( mkdir( pszPath, 0777 ) < 0 ) {
					#endif
						pClient->SendReply( "550 MKD Error Creating DIR." );
					} else
						pClient->SendReply( "250 MKD command successful." );
				} else
					pClient->SendReply( "550 File Already Exists." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else if( nCmd == CMD_RMD || nCmd == CMD_XRMD ) {

			if( !pClient->CheckPrivileges( CFtpServer::DELETEDIR ) ) {
				pClient->SendReply( "550 Permission denied." );
				continue;
			}
			if( pszCmdArg ) {
				pszPath = pClient->BuildPath( pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					#ifdef WIN32
						if ( _rmdir( pszPath ) == -1 ) {
					#else
						if ( rmdir( pszPath ) < 0 ) {
					#endif
						pClient->SendReply( "450 Internal error deleting the directory." );
					} else
						pClient->SendReply( "250 Directory deleted successfully." );
				} else
					pClient->SendReply( "550 Directory not found." );
			} else
				pClient->SendReply( "501 Syntax error in arguments." );
			continue;

		} else {
			pClient->SendReply( "500 Command not understood." );
			continue;
		}

	}
	if( pszPath ) delete [] pszPath;

	#ifdef WIN32
		CloseHandle( pClient->hClientThread );
	#else
		pthread_detach( pClient->ClientThreadID );
	#endif

	pFtpServer->ClientListLock.Enter();
	{
		pClient->ClientLock.Enter();
		{
			if( pClient->CtrlSock != INVALID_SOCKET ) {
				SOCKET tmpSock = pClient->CtrlSock;
				pClient->CtrlSock = INVALID_SOCKET;
				CloseSocket( tmpSock );
			}
		}
		pClient->ClientLock.Leave();
		if( pClient->eStatus == WAITING ) {
			delete pClient;
		} else
			pClient->bIsCtrlCanalOpen = false;
	}
	pFtpServer->ClientListLock.Leave();

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

void CFtpServer::CClientEntry::LogIn()
{
	bIsLogged = true;
	++pUser->uiNumberOfClient;
	SendReply( "230 User Logged In." );
	ResetTimeout();
}

void CFtpServer::CClientEntry::LogOut()
{
	if( pUser ) {
		--pUser->uiNumberOfClient;
		pUser = NULL;
	}
	bIsLogged = false;
	strcpy( szWorkingDir, "/" );
	if( szRenameFromPath )
		*szRenameFromPath = 0x0;
}

////////////////////////////////////////
// CONTROL CHANNEL
////////////////////////////////////////

bool CFtpServer::CClientEntry::SendReply( const char *pszReply, bool bNoNeedToAlloc /* =0 */ )
{
	if( pszReply ) {
		bool bReturn = false;
		int nLen = strlen( pszReply );
		char *pszBuffer;
		if( !bNoNeedToAlloc ) {
			pszBuffer = new char[ nLen + 2 ];
			if( !pszBuffer ) {
				pFtpServer->OnServerEventCb( MEM_ERROR );
				return false;
			}
			memcpy( pszBuffer, pszReply, nLen );
		} else pszBuffer = (char*) pszReply;
		pFtpServer->OnClientEventCb( SEND_REPLY, this, (void*)pszReply );
		pszBuffer[ nLen  ] = '\r';
		pszBuffer[ nLen + 1 ] = '\n';
		if( send( CtrlSock, pszBuffer, nLen + 2, MSG_NOSIGNAL ) > 0 ) {
			bReturn = true;
		} else
			pFtpServer->OnClientEventCb( CLIENT_SOCK_ERROR, this );
		delete [] pszBuffer;
		return bReturn;
	}
	return false;
}

bool CFtpServer::CClientEntry::SendReply2( const char *pszList, ... )
{
	if( pszList ) {
		char *pszBuffer = new char[ CFTPSERVER_REPLY_MAX_LEN ];
		if( pszBuffer ) {
			va_list args;
			va_start( args, pszList );
			vsnprintf( pszBuffer, CFTPSERVER_REPLY_MAX_LEN - 2, pszList , args);
			return SendReply( pszBuffer, 1 );
		} else
			pFtpServer->OnServerEventCb( MEM_ERROR );
	}
	return false;
}

bool CFtpServer::CClientEntry::ReceiveLine()
{

	iCmdRecvdLen = 0;
	pszCmdArg = NULL;
	iCmdLen = 0;
	if( psNextCmd && nRemainingCharToParse > 0 ) {
		memmove( &sCmdBuffer, psNextCmd, nRemainingCharToParse * sizeof( char ) );
		psNextCmd = NULL;
	}

	double dt;
	int iRemainingCmdLen;
	fd_set fdRead;
	struct timeval *ptv, tv;
	bool bFlushNextLine = false;

	for( ;; ) {

		FD_ZERO( &fdRead );
		FD_SET( CtrlSock, &fdRead );

		ptv = NULL; tv.tv_sec = 0; tv.tv_usec = 0;
		if( !IsLogged() && pFtpServer->GetNoLoginTimeout() ) {
			ptv = &tv;
			dt = difftime( time( NULL ), tTimeoutTime );
			if( dt < pFtpServer->GetNoLoginTimeout() )
				tv.tv_sec = (long)( pFtpServer->GetNoLoginTimeout() - dt );
		} else if( IsLogged() && pFtpServer->GetNoTransferTimeout() ) {
			ptv = &tv;
			dt = difftime( time( NULL ), tTimeoutTime );
			if( dt < pFtpServer->GetNoTransferTimeout() )
				tv.tv_sec = (long)( pFtpServer->GetNoTransferTimeout() - dt );
		}

		if( nRemainingCharToParse > 0 ) {
			char *s;
			while( nRemainingCharToParse ) {
				--nRemainingCharToParse;
				s = sCmdBuffer + iCmdRecvdLen;
				if( *s == '\r' || *s == '\n' || *s == 0 ) {
					if( iCmdRecvdLen > 0 && !bFlushNextLine ) {
						*s = 0x0;
						iCmdLen = iCmdRecvdLen;
						psNextCmd = s + 1;
						break;
					} else {
						memmove( s, s + 1, nRemainingCharToParse );
						if( bFlushNextLine ) {
							iCmdRecvdLen = 0;
							bFlushNextLine = false;
						}
					}
				} else
					++iCmdRecvdLen;
			}
			if( iCmdLen ) {
				pFtpServer->OnClientEventCb( RECVD_CMD_LINE, this, sCmdBuffer );
				return true;
			}
		}

		iRemainingCmdLen = MAX_COMMAND_LINE_LEN - iCmdRecvdLen;
		if( iRemainingCmdLen <= 0 ) {
			if( !bFlushNextLine )
				SendReply( "500 Command line is too long !" );
			iCmdRecvdLen = 0;
			bFlushNextLine = true;
			iRemainingCmdLen = MAX_COMMAND_LINE_LEN;
		}

		if( select( CtrlSock + 1, &fdRead, NULL, NULL, ptv ) == SOCKET_ERROR )
			break;

		if( !FD_ISSET( CtrlSock, &fdRead ) ) {
			dt = difftime( time( NULL ), tTimeoutTime );
			if( ( !IsLogged() && pFtpServer->GetNoLoginTimeout() )
				|| ( eStatus == WAITING && pFtpServer->GetNoTransferTimeout()
				&& dt >= pFtpServer->GetNoTransferTimeout() ) )
			{
				SendReply( "421 Timeout: closing control connection." );
				pFtpServer->OnClientEventCb( IsLogged() ? NO_TRANSFER_TIMEOUT : NO_LOGIN_TIMEOUT, this );
				return false;
			} else
				continue;
		}
		
		int Err = recv( CtrlSock, sCmdBuffer + iCmdRecvdLen , iRemainingCmdLen, 0 );
		if( Err > 0 ) {
			nRemainingCharToParse += Err;
		} else if( Err == 0 ) {
			pFtpServer->OnClientEventCb( CLIENT_DISCONNECT, this );
			break;
		} else if( Err == SOCKET_ERROR ) {
			pFtpServer->OnClientEventCb( CLIENT_SOCK_ERROR, this );
			break;
		}

	}
	return false;
}

int CFtpServer::CClientEntry::ParseLine()
{
	// Separate the Cmd and the Arguments
	char *pszSpace = strchr( sCmdBuffer, ' ' );
	if( pszSpace ) { 
		pszSpace[0] = '\0';
		pszCmdArg = pszSpace + 1;
	} else pszCmdArg = NULL;

	// Convert the Cmd to uppercase
	#ifdef WIN32
		_strupr( sCmdBuffer );
	#else
		char *psToUpr = sCmdBuffer;
		while(	psToUpr && psToUpr[0] ) {
			*psToUpr = toupper( *psToUpr );
			++psToUpr;
		}
	#endif

	switch( sCmdBuffer[0] ) {
		case 'A':
			switch( sCmdBuffer[1] ) {
				case 'B':
					if( !strcmp( sCmdBuffer + 2, "OR" ) ) return CMD_ABOR;
					break;
				case 'L':
					if( !strcmp( sCmdBuffer + 2, "LO" ) ) return CMD_ALLO;
					break;
				case 'P':
					if( !strcmp( sCmdBuffer + 2, "PE" ) ) return CMD_APPE;
					break;
			}
			break;
		case 'C':
			switch( sCmdBuffer[1] ) {
				case 'D':
					if( !strcmp( sCmdBuffer + 2, "UP" ) ) return CMD_CDUP;
					break;
				case 'L':
					if( !strcmp( sCmdBuffer + 2, "NT" ) ) return CMD_CLNT;
					break;
				case 'W':
					if( !strcmp( sCmdBuffer + 2, "D" ) ) return CMD_CWD;
					break;
			}
			break;
		case 'D':
			if( !strcmp( sCmdBuffer + 1, "ELE" ) ) return CMD_DELE;
			break;
		case 'F':
			if( !strcmp( sCmdBuffer + 1, "EAT" ) ) return CMD_FEAT;
			break;
		case 'H':
			if( !strcmp( sCmdBuffer + 1, "ELP" ) ) return CMD_HELP;
			break;
		case 'L':
			if( !strcmp( sCmdBuffer + 1, "IST" ) ) return CMD_LIST;
			break;
		case 'M':
			switch( sCmdBuffer[1] ) {
				case 'D':
					if( !strcmp( sCmdBuffer + 2, "TM" ) ) return CMD_MDTM;
					break;
				case 'K':
					if( !strcmp( sCmdBuffer + 2, "D" ) ) return CMD_MKD;
					break;
				case 'O':
					if( !strcmp( sCmdBuffer + 2, "DE" ) ) return CMD_MODE;
					break;
			}
			break;
		case 'N':
			switch( sCmdBuffer[1] ) {
				case 'L':
					if( !strcmp( sCmdBuffer + 2, "ST" ) ) return CMD_NLST;
					break;
				case 'O':
					if( !strcmp( sCmdBuffer + 2, "OP" ) ) return CMD_NOOP;
					break;
			}
			break;
		case 'O':
			if( !strcmp( sCmdBuffer + 1, "PTS" ) ) return CMD_OPTS;
			break;
		case 'P':
			switch( sCmdBuffer[1] ) {
				case 'A':
					if( sCmdBuffer[2] == 'S' ) {
						switch( sCmdBuffer[3] ) {
							case 'V':
								if( sCmdBuffer[4] == 0 ) return CMD_PASV;
								break;
							case 'S':
								if( sCmdBuffer[4] == 0 ) return CMD_PASS;
								break;
						}
					}
					break;
				case 'O':
					if( !strcmp( sCmdBuffer + 2, "RT" ) ) return CMD_PORT;
					break;
				case 'W':
					if( !strcmp( sCmdBuffer + 2, "D" ) ) return CMD_PWD;
					break;
			}
			break;
		case 'Q':
			if( !strcmp( sCmdBuffer + 1, "UIT" ) ) return CMD_QUIT;
			break;
		case 'R':
			switch( sCmdBuffer[1] ) {
				case 'E':
					switch( sCmdBuffer[2] ) {
						case 'S':
							if( !strcmp( sCmdBuffer + 3, "T" ) ) return CMD_REST;
							break;
						case 'T':
							if( !strcmp( sCmdBuffer + 3, "R" ) ) return CMD_RETR;
							break;
					}
					break;
				case 'M':
					if( !strcmp( sCmdBuffer + 2, "D" ) ) return CMD_RMD;
					break;
				case 'N':
					switch( sCmdBuffer[2] ) {
						case 'F':
							if( !strcmp( sCmdBuffer + 3, "R" ) ) return CMD_RNFR;
							break;
						case 'T':
							if( !strcmp( sCmdBuffer + 3, "O" ) ) return CMD_RNTO;
							break;
					}
					
					break;
			}
			break;
		case 'S':
			switch( sCmdBuffer[1] ) {
				case 'I':
					switch( sCmdBuffer[2] ) {
						case 'T':
							if( !strcmp( sCmdBuffer + 3, "E" ) ) return CMD_SITE;
							break;
						case 'Z':
							if( !strcmp( sCmdBuffer + 3, "E" ) ) return CMD_SIZE;
							break;
					}
					break;
				case 'T':
					switch( sCmdBuffer[2] ) {
						case 'A':
							if( !strcmp( sCmdBuffer + 3, "T" ) ) return CMD_STAT;
							break;
						case 'O':
							switch( sCmdBuffer[3] ) {
								case 'R':
									if( sCmdBuffer[4] == '\0' ) return CMD_STOR;
									break;
								case 'U':
									if( sCmdBuffer[4] == '\0' ) return CMD_STOU;
									break;
							}
						case 'R':
							if( !strcmp( sCmdBuffer + 3, "U" ) ) return CMD_STRU;
							break;
					}
					break;
				case 'Y':
					if( !strcmp( sCmdBuffer + 2, "ST" ) ) return CMD_SYST;
					break;
			}
			break;

			break;
		case 'T':
			if( !strcmp( sCmdBuffer + 1, "YPE" ) ) return CMD_TYPE;
			break;
		case 'U':
			if( !strcmp( sCmdBuffer + 1, "SER" ) ) return CMD_USER;
			break;
		case 'X':
			switch( sCmdBuffer[1] ) {
				case 'C':
					switch( sCmdBuffer[2] ) {
						case 'U':
							if( !strcmp( sCmdBuffer + 3, "P" ) ) return CMD_XCUP;
							break;
						case 'W':
							if( !strcmp( sCmdBuffer + 3, "D" ) ) return CMD_XCWD;
							break;
					}
					break;
				case 'M':
					if( !strcmp( sCmdBuffer + 2, "KD" ) ) return CMD_XMKD;
				case 'R':
					if( !strcmp( sCmdBuffer + 2, "MD" ) ) return CMD_XRMD;
				case 'P':
					if( !strcmp( sCmdBuffer + 2, "PW" ) ) return CMD_XPWD;
			}
			break;
	}
	return CMD_NONE;
}

////////////////////////////////////////
// DATA CHANNEL
////////////////////////////////////////

bool CFtpServer::CClientEntry::OpenDataConnection( int nCmd )
{
	if( eStatus != WAITING ) {
		SendReply( "425 You're already connected." );
		return false;
	}
	if( eDataConnection == NONE ) {
		SendReply( "503 Bad sequence of commands." );
		return false;
	}

	if( eDataConnection == PORT ) {
		SendReply( "150 Opening data channel." );
		DataSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	}

	int iBufSize = pFtpServer->GetTransferSocketBufferSize();

	if( DataSock != INVALID_SOCKET && !setsockopt( DataSock, SOL_SOCKET,
		( nCmd == CMD_STOR ) ? SO_RCVBUF : SO_SNDBUF, (char*)&iBufSize, sizeof(int) ) )
	{

		if( eDataConnection == PASV ) {

			SOCKET sTmpWs;
			struct sockaddr_in sin;
			socklen_t sin_len = sizeof( struct sockaddr_in );
			#ifdef USE_BSDSOCKETS
				sin.sin_len = sizeof( sin );
			#endif

			fd_set fdRead;
			FD_ZERO( &fdRead );
			FD_SET( DataSock, &fdRead );
			timeval tv;	tv.tv_sec = 10; tv.tv_usec= 0;

			if( select( DataSock + 1, &fdRead, NULL, NULL, &tv ) > 0 && FD_ISSET( DataSock, &fdRead ) )
			{
				sTmpWs = accept( DataSock, (struct sockaddr *) &sin, &sin_len);
				CloseSocket( DataSock );
				DataSock = sTmpWs;
				if( sTmpWs != INVALID_SOCKET ) {
					SendReply( "150 Connection accepted." );
					return true;
				}
			}

		} else { // eDataConnection == PORT

			struct sockaddr_in BindSin;
			BindSin.sin_family = AF_INET;
			BindSin.sin_addr.s_addr = ulServerIP;
			#ifdef USE_BSDSOCKETS
				BindSin.sin_len = sizeof( BindSin );
			#endif
			unsigned short int usLen, usStart;
			pFtpServer->GetDataPortRange( &usStart, &usLen );
			BindSin.sin_port = (unsigned short) ( usStart + ( rand() % usLen ) );

			int on = 1; // Here the strange behaviour of SO_REUSEADDR under win32 is welcome.
			#ifdef SO_REUSEPORT
				(void) setsockopt( DataSock, SOL_SOCKET, SO_REUSEPORT, (char *) &on, sizeof on);
			#else
				(void) setsockopt( DataSock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof on);
			#endif

			if( bind( DataSock, (struct sockaddr *) &BindSin, sizeof( struct sockaddr_in ) ) != SOCKET_ERROR ) {

				struct sockaddr_in ConnectSin;
				ConnectSin.sin_family = AF_INET;
				ConnectSin.sin_port = htons( usDataPort );
				ConnectSin.sin_addr.s_addr = ulDataIp;
				#ifdef USE_BSDSOCKETS
					ConnectSin.sin_len = sizeof( ConnectSin );
				#endif

				if( connect( DataSock, (struct sockaddr *)&ConnectSin, sizeof( ConnectSin ) ) != SOCKET_ERROR )
					return true;

			} // else Internal Error
		}
	}
	SendReply( "425 Can't open data connection." );
	ResetDataConnection();
	return false;
}

bool CFtpServer::CClientEntry::ResetDataConnection( bool bSyncWait /* =true */ )
{
	ClientLock.Enter();
	{
		if( DataSock != INVALID_SOCKET ) {
			SOCKET tmpSock = DataSock;
			DataSock = CurrentTransfer.SockList = INVALID_SOCKET;
			CloseSocket( tmpSock );
		}
#ifdef WIN32
		if( CurrentTransfer.hTransferThread != INVALID_HANDLE_VALUE ) {
			if( bSyncWait ) {
				ClientLock.Leave();
				WaitForSingleObject( CurrentTransfer.hTransferThread, INFINITE );
				ClientLock.Enter();
			}
			CloseHandle( CurrentTransfer.hTransferThread );
		}
#else
		if( CurrentTransfer.TransferThreadID ) {
			if( bSyncWait ) {
				ClientLock.Leave();
				if( !pthread_cancel( CurrentTransfer.TransferThreadID ) )
					pthread_join( CurrentTransfer.TransferThreadID, 0 );
				ClientLock.Enter();
			} else
				pthread_detach( CurrentTransfer.TransferThreadID );
		}
#endif
		memset( &CurrentTransfer, 0x0, sizeof( CurrentTransfer ) );
#ifdef WIN32
		CurrentTransfer.hTransferThread = INVALID_HANDLE_VALUE;
#endif
		eStatus = WAITING;
		eDataConnection = NONE;
	}
	ClientLock.Leave();
	return true;
}

////////////////////////////////////////
// TRANSFER
////////////////////////////////////////

#ifdef CFTPSERVER_ENABLE_ZLIB
bool CFtpServer::CClientEntry::InitZlib( DataTransfer_t *pTransfer )
{
	int nRet;
	pTransfer->zStream.zfree = Z_NULL;
	pTransfer->zStream.zalloc = Z_NULL;
	pTransfer->zStream.opaque = Z_NULL;
	CFtpServer *pFtpServer = pTransfer->pClient->pFtpServer;

	switch( pTransfer->pClient->eStatus ) {
		case LISTING:
		case DOWNLOADING:
			nRet = deflateInit( &pTransfer->zStream, pTransfer->iZlibLevel );
			if( nRet == Z_OK )
				return true;
			break;
		case UPLOADING:
			nRet = inflateInit( &pTransfer->zStream );
			if( nRet == Z_OK )
				return true;
			break;
	}

	switch( nRet ) {
		case Z_MEM_ERROR:
			pFtpServer->OnServerEventCb( MEM_ERROR );
			break;
		case Z_VERSION_ERROR:
			pFtpServer->OnServerEventCb( ZLIB_VERSION_ERROR );
			break;
		case Z_STREAM_ERROR:
			pFtpServer->OnServerEventCb( ZLIB_STREAM_ERROR );
			break;
	}
	return false;
}
#endif

bool CFtpServer::CClientEntry::SafeWrite( int hFile, char *pBuffer, int nLen )
{
	int wl = 0, k;
	while(wl != nLen ) {
		k = write( hFile, pBuffer + wl, nLen - wl );
		if(k < 0)
		     return false;
		wl += k;
	}
	return true;
}

#ifdef WIN32
	unsigned CFtpServer::CClientEntry::StoreThread( void *pvParam )
#else
	void* CFtpServer::CClientEntry::StoreThread( void *pvParam )
#endif
{
	CFtpServer::CClientEntry *pClient = (CFtpServer::CClientEntry*) pvParam;
	CFtpServer *pFtpServer = pClient->pFtpServer;
	struct DataTransfer_t *pTransfer = &pClient->CurrentTransfer;

	int len = 0; int hFile = -1;

	int iflags = O_WRONLY | O_CREAT | O_BINARY;
	if( pClient->CurrentTransfer.RestartAt > 0 ) {
		iflags |= O_APPEND; //a|b
	} else
		iflags |= O_TRUNC; //w|b

	unsigned int uiBufferSize = pFtpServer->GetTransferBufferSize();
	char *pBuffer = new char[ uiBufferSize ];

#ifdef CFTPSERVER_ENABLE_ZLIB
	int nFlush, nRet;
	char *pOutBuffer = NULL;
	if( pTransfer->eDataMode == ZLIB ) {
		pOutBuffer = new char[ uiBufferSize ];
		if( !pOutBuffer || !pBuffer ) {
			pFtpServer->OnServerEventCb( MEM_ERROR );
			goto endofstore;
		}
		if( !pClient->InitZlib( pTransfer ) )
			goto endofstore;
		pTransfer->zStream.next_out = (Bytef*) pOutBuffer;
		pTransfer->zStream.avail_out = uiBufferSize;
	}
#else
	if( !pBuffer ) {
		pFtpServer->OnServerEventCb( MEM_ERROR );
		goto endofstore;
	}
#endif

	hFile = open( pTransfer->szPath, iflags, (int)0777 );

	if( hFile >= 0 ) {
		if( (pTransfer->RestartAt > 0 && lseek( hFile, pTransfer->RestartAt, SEEK_SET) != -1 )
			|| pTransfer->RestartAt == 0 )
		{
			fd_set fdRead;
			char *ps = pBuffer;
			char *pe = pBuffer + uiBufferSize;

			while( pClient->DataSock != INVALID_SOCKET ) {

				FD_ZERO( &fdRead );
				FD_SET( pClient->DataSock, &fdRead );
	
				if( select( pClient->DataSock + 1, &fdRead, NULL, NULL, NULL ) > 0
					&& FD_ISSET( pClient->DataSock, &fdRead ) )
				{
					len = recv( pClient->DataSock, ps, (pe - ps), 0 );
					if( len >= 0 ) {

#ifdef CFTPSERVER_ENABLE_ZLIB
						if( pTransfer->eDataMode == ZLIB ) {
							pTransfer->zStream.avail_in = len;
							pTransfer->zStream.next_in = (Bytef*) pBuffer;
							nFlush = !len ? Z_NO_FLUSH : Z_FINISH;
							do
							{
								nRet = inflate( &pTransfer->zStream, nFlush );
								if( nRet != Z_OK && nRet != Z_STREAM_END && nRet != Z_BUF_ERROR )
									break; // Zlib error
								if( len == 0 && nRet == Z_BUF_ERROR )
									break; // transfer has been interrupt by the client.
								if( pTransfer->zStream.avail_out == 0 || nRet == Z_STREAM_END ) {
									if( !pClient->SafeWrite( hFile, pOutBuffer, uiBufferSize - pTransfer->zStream.avail_out) ) {
										len = -1;
										break; // write error
									}
									pTransfer->zStream.next_out = (Bytef*) pOutBuffer;
									pTransfer->zStream.avail_out = uiBufferSize;
								}
							} while( pTransfer->zStream.avail_in != 0 );
							if( nRet == Z_STREAM_END )
								break;
						} else
#endif
						if( len > 0 ) {
							ps += len;
							if( ps == pe ) {
								if( !pClient->SafeWrite( hFile, pBuffer, ps - pBuffer ) ) {
									len = -1;
									break;
								}
								ps = pBuffer;
							}
						} else {
							pClient->SafeWrite( hFile, pBuffer, ps - pBuffer );
							break;
						}
					} else
						break; // Socket Error

				} else {
					len = -1;
					break;
				}
 			}
		}
		close( hFile );
	}

endofstore:
	if( pBuffer ) delete [] pBuffer;
#ifdef CFTPSERVER_ENABLE_ZLIB
	if( pTransfer->eDataMode == ZLIB ) {
		deflateEnd( &pTransfer->zStream );
		if( pOutBuffer ) delete [] pOutBuffer;
	}
#endif

	if( len < 0 || hFile == -1 ) {
		pClient->SendReply( "550 Can't store file." );
	} else
		pClient->SendReply( "226 Transfer complete." );

	pFtpServer->ClientListLock.Enter();
	{
		pClient->ResetDataConnection( false ); // do not wait sync on self
		if( pClient->bIsCtrlCanalOpen == true ) {
			pClient->ResetTimeout();
		} else
			delete pClient;
	}
	pFtpServer->ClientListLock.Leave();

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

#ifdef WIN32
	unsigned CFtpServer::CClientEntry::RetrieveThread( void *pvParam )
#else
	void* CFtpServer::CClientEntry::RetrieveThread( void *pvParam )
#endif
{
	CFtpServer::CClientEntry *pClient = (CFtpServer::CClientEntry*) pvParam;
	CFtpServer *pFtpServer = pClient->pFtpServer;
	struct DataTransfer_t *pTransfer = &pClient->CurrentTransfer;

	int hFile = -1;
	int BlockSize = 0; int len = 0;

	unsigned int uiBufferSize = pFtpServer->GetTransferBufferSize();
	char *pBuffer = new char[ uiBufferSize ];

#ifdef CFTPSERVER_ENABLE_ZLIB
	int nFlush, nRet;
	char *pOutBuffer = NULL;
	if( pTransfer->eDataMode == ZLIB ) {
		pOutBuffer = new char[ uiBufferSize ];
		if( !pOutBuffer || !pBuffer ) {
			pFtpServer->OnServerEventCb( MEM_ERROR );
			goto endofretrieve;
		}
		if( !pClient->InitZlib( pTransfer ) )
			goto endofretrieve;
		pTransfer->zStream.next_in = (Bytef*) pBuffer;
		pTransfer->zStream.next_out = (Bytef*) pOutBuffer;
		pTransfer->zStream.avail_out = uiBufferSize;
	}
#else
	if( !pBuffer ) {
		pFtpServer->OnServerEventCb( MEM_ERROR );
		goto endofretrieve;
	}
#endif

	hFile = open( pTransfer->szPath, O_RDONLY | O_BINARY );
	if( hFile != -1 ) {
		if( pTransfer->RestartAt == 0
			|| ( pTransfer->RestartAt > 0 && lseek( hFile, pTransfer->RestartAt, SEEK_SET ) != -1 ) )
		{
			while( pClient->DataSock != INVALID_SOCKET
				&& (BlockSize = read( hFile, pBuffer, uiBufferSize )) > 0
			) {
#ifdef CFTPSERVER_ENABLE_ZLIB
				if( pTransfer->eDataMode == ZLIB ) {
					nFlush = eof( hFile ) ? Z_FINISH : Z_NO_FLUSH;
					pTransfer->zStream.avail_in = BlockSize;
					pTransfer->zStream.next_in = (Bytef*) pBuffer;
					do
					{
						nRet = deflate( &pTransfer->zStream, nFlush );
						if( nRet == Z_STREAM_ERROR )
							break;
						if( pTransfer->zStream.avail_out == 0 || nRet == Z_STREAM_END ) {
							len = send( pClient->DataSock, pOutBuffer, uiBufferSize - pTransfer->zStream.avail_out,	MSG_NOSIGNAL );
							if( len <= 0 )
								{ len =-1; break; }
							pTransfer->zStream.next_out = (Bytef*) pOutBuffer;
							pTransfer->zStream.avail_out = uiBufferSize;
						}
					} while( pTransfer->zStream.avail_in != 0 || ( nFlush == Z_FINISH && nRet == Z_OK ) );
					if( len < 0 || nRet == Z_STREAM_ERROR || nFlush == Z_FINISH )
						break;
				} else
#endif
				{
					len = send( pClient->DataSock, pBuffer, BlockSize, MSG_NOSIGNAL );
					if( len <= 0 ) break;
				}
			}
		} // else Internal Error
		close( hFile );
	}

endofretrieve:
	if( pBuffer ) delete [] pBuffer;
#ifdef CFTPSERVER_ENABLE_ZLIB
	if( pTransfer->eDataMode == ZLIB ) {
		deflateEnd( &pTransfer->zStream );
		if( pOutBuffer ) delete [] pOutBuffer;
	}
#endif

	if( len < 0 || hFile == -1 ) {
		pClient->SendReply( "550 Can't retrieve File." );
	} else
		pClient->SendReply( "226 Transfer complete." );

	pFtpServer->ClientListLock.Enter();
	{
		pClient->ResetDataConnection( false ); // do not wait sync on self
		if( pClient->bIsCtrlCanalOpen == true ) {
			pClient->ResetTimeout();
		} else
			delete pClient;
	}
	pFtpServer->ClientListLock.Leave();

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

// !!!  psLine must be at least CFTPSERVER_LIST_MAX_LINE_LEN (=MAX_PATH+57) char long.
int CFtpServer::CClientEntry::GetFileListLine( char* psLine, unsigned short mode,
#ifdef __USE_FILE_OFFSET64
	__int64 size,
#else
	long size,
#endif
	time_t mtime, const char* pszName, bool opt_F )
{

	if( !psLine || !pszName )
		return -1;

	char szYearOrHour[ 6 ] = "";

	static const char *pszMonth[] =
		{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	static const char szListFile[] = "%c%c%c%c%c%c%c%c%c%c 1 user group %14"
#ifdef __USE_FILE_OFFSET64
	#ifdef WIN32
									"I64"
	#else
									"ll"
	#endif
#else
									"l"
#endif
									"i %s %2d %s %s%s\r\n";

	struct tm *t = gmtime( (time_t *) &mtime ); // UTC Time
	if( time(NULL) - mtime > 180 * 24 * 60 * 60 ) {
		sprintf( szYearOrHour, "%5d", t->tm_year + 1900 );
	} else
		sprintf( szYearOrHour, "%02d:%02d", t->tm_hour, t->tm_min );

	int iLineLen = sprintf( psLine, szListFile,
		( S_ISDIR( mode ) ) ? 'd' : '-',
		( mode & S_IREAD ) == S_IREAD ? 'r' : '-',
		( mode & S_IWRITE ) == S_IWRITE ? 'w' : '-',
		( mode & S_IEXEC ) == S_IEXEC ? 'x' : '-',
		'-', '-', '-', '-', '-', '-',
		size,
		pszMonth[ t->tm_mon ], t->tm_mday, szYearOrHour,
		pszName,
		( opt_F && S_ISDIR( mode ) ? "/" : "" ) );        
            
	return iLineLen;
}

bool CFtpServer::CClientEntry::AddToListBuffer( DataTransfer_t *pTransfer,
				char *pszListLine, int nLineLen,
				char *pBuffer, unsigned int *nBufferPos, unsigned int uiBufferSize )
{

#ifdef CFTPSERVER_ENABLE_ZLIB
	if( pTransfer->eDataMode == ZLIB ) {
		int nRet;
		pTransfer->zStream.avail_in = nLineLen;
		pTransfer->zStream.next_in = (Bytef*) pszListLine;
		int nFlush = pszListLine ? Z_NO_FLUSH : Z_FINISH;
		do
		{
			nRet = deflate( &pTransfer->zStream, nFlush );
			if( nRet == Z_STREAM_ERROR )
				return false;
			if( pTransfer->zStream.avail_out == 0 || nRet == Z_STREAM_END ) {
				*nBufferPos = uiBufferSize - pTransfer->zStream.avail_out;
				if( send( pTransfer->SockList, pBuffer, *nBufferPos, MSG_NOSIGNAL ) <= 0 )
					return false;
				*nBufferPos = 0;
				pTransfer->zStream.next_out = (Bytef*) pBuffer;
				pTransfer->zStream.avail_out = uiBufferSize;
			}
		} while( pTransfer->zStream.avail_in != 0 || ( nFlush == Z_FINISH && nRet == Z_OK ) );
		*nBufferPos = uiBufferSize - pTransfer->zStream.avail_out;
	} else
#endif
	{
		if( pszListLine ) {
			int nBufferAvailable = uiBufferSize - *nBufferPos;
			int iCanCopyLen = (nLineLen <= nBufferAvailable) ? nLineLen : nBufferAvailable;
			memcpy( (pBuffer + *nBufferPos), pszListLine, iCanCopyLen );
			*nBufferPos += iCanCopyLen;
			if( *nBufferPos == uiBufferSize ) {
				if( send( pTransfer->SockList, pBuffer, uiBufferSize, MSG_NOSIGNAL ) <= 0 )
					return false;
				*nBufferPos = 0;
				if( iCanCopyLen < nLineLen ) {
					memcpy( pBuffer, pszListLine + iCanCopyLen, nLineLen - iCanCopyLen );
					*nBufferPos = (nLineLen - iCanCopyLen);
				}
			}
		} else { // Flush the buffer.
			if( nBufferPos )
				send( pTransfer->SockList, pBuffer, *nBufferPos, MSG_NOSIGNAL );
		} 
	}
	return true;
}

#ifdef WIN32
	unsigned CFtpServer::CClientEntry::ListThread( void *pvParam )
#else
	void* CFtpServer::CClientEntry::ListThread( void *pvParam )
#endif
{
	CFtpServer::CClientEntry *pClient = (CFtpServer::CClientEntry*) pvParam;
	CFtpServer *pFtpServer = pClient->pFtpServer;
	struct DataTransfer_t *pTransfer = &pClient->CurrentTransfer;
	struct stat *st = &pTransfer->st;

	int iFileLineLen = 0;
	char *psFileLine = new char[ CFTPSERVER_LIST_MAX_LINE_LEN ];

	if( psFileLine ) {

		if( pTransfer->opt_d || !S_ISDIR( st->st_mode ) ) {

			char *pszName = strrchr( pTransfer->szPath, '/' );
			iFileLineLen = pClient->GetFileListLine( psFileLine, st->st_mode, st->st_size,
				st->st_mtime, (( pszName && pszName[ 1 ] ) ? pszName + 1 : "."), pTransfer->opt_F );

			if( iFileLineLen > 0 )
				send( pTransfer->SockList, psFileLine, iFileLineLen, MSG_NOSIGNAL );

		} else {

			CEnumFileInfo *fi = new CFtpServer::CEnumFileInfo;
			unsigned int uiBufferSize = pFtpServer->GetTransferBufferSize();
			char *pBuffer = new char[ uiBufferSize ];
			unsigned int nBufferPos = 0;

			if( !pBuffer || !fi ) {
				pFtpServer->OnServerEventCb( MEM_ERROR );
				goto endoflist;
			}

#ifdef CFTPSERVER_ENABLE_ZLIB
			if( pTransfer->eDataMode == ZLIB ) {
				if( !pClient->InitZlib( pTransfer ) ) 
					goto endoflist;
				pTransfer->zStream.next_in = (Bytef*) psFileLine;
				pTransfer->zStream.next_out = (Bytef*) pBuffer;
				pTransfer->zStream.avail_out = uiBufferSize;
			}
#endif
			if( fi->FindFirst( pTransfer->szPath ) ) {
				do
				{
					if( pClient->CurrentTransfer.nCmd ==  CMD_NLST ) {

						if( fi->pszName[0] != '.' || pTransfer->opt_a ) {
							if( !pClient->AddToListBuffer( pTransfer, fi->pszName, strlen( fi->pszName ), pBuffer, &nBufferPos, uiBufferSize ) )
								break;
						}

					} else if( fi->pszName[0] != '.' || pTransfer->opt_a ) {

						iFileLineLen = pClient->GetFileListLine( psFileLine, fi->mode,
							fi->size, fi->mtime, fi->pszName, pTransfer->opt_F );

						if( !pClient->AddToListBuffer( pTransfer, psFileLine, iFileLineLen, pBuffer, &nBufferPos, uiBufferSize ) )
							break;

					}

				} while( fi->FindNext() );
				fi->FindClose();

				// Flush the buffer
				pClient->AddToListBuffer( pTransfer, NULL, 0, pBuffer, &nBufferPos, uiBufferSize );

			} // else Directory doesn't exist anymore..

endoflist:
			if( fi ) delete fi;
			if( pBuffer ) delete [] pBuffer;
#ifdef CFTPSERVER_ENABLE_ZLIB
			if( pTransfer->eDataMode == ZLIB )
				deflateEnd( &pTransfer->zStream );
#endif
		}
		delete [] psFileLine;

	} else
		pFtpServer->OnServerEventCb( MEM_ERROR );

	if( pTransfer->nCmd != CMD_STAT ) {
		pClient->SendReply( "226 Transfer complete." );
	} else
		pClient->SendReply( "213 End of status" );

	pFtpServer->ClientListLock.Enter();
	{
		pClient->ResetDataConnection( false ); // do not wait sync on self
		if( pClient->bIsCtrlCanalOpen == true ) {
			pClient->ResetTimeout();
		} else
			delete pClient;
	}
	pFtpServer->ClientListLock.Leave();

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

//////////////////////////////////////////////////////////////////////
// CFtpServer::CEnumFileInfo CLASS
//////////////////////////////////////////////////////////////////////

bool CFtpServer::CEnumFileInfo::FindFirst( const char *pszPath )
{
	if( pszPath ) {
		strcpy( szDirPath, pszPath );
		#ifdef WIN32
			hFile = -1L;
			int iPathLen = strlen( pszPath );
			sprintf( pszTempPath, "%s%s*", pszPath, ( pszPath[ iPathLen - 1 ] != '/' ) ? "/" : "" );
			if( FindNext() )
				return true;
		#else
			if( ( dp = opendir( pszPath ) ) != NULL ) {
				if( FindNext() )
					return true;
			}
		#endif
	}
	return false;
}

bool CFtpServer::CEnumFileInfo::FindNext()
{
	#ifdef WIN32
		bool i = false;
		if( hFile == -1L ) {
			hFile = _findfirst( pszTempPath, &c_file );
			if( hFile != -1 )
				i = true;
		} else if( _findnext( hFile, &c_file ) == 0 )
			i = true;
		if( i ) {
			int iDirPathLen = strlen( szDirPath );
			int iFileNameLen = strlen( c_file.name );
			if( iDirPathLen + iFileNameLen >= MAX_PATH )
				return false;
			sprintf( szFullPath, "%s%s%s", szDirPath,
				( szDirPath[ iDirPathLen - 1 ] == '/' ) ? "" : "/",
				c_file.name );
			pszName = szFullPath + iDirPathLen +
					( ( szDirPath[ iDirPathLen - 1 ] != '/' ) ? 1 : 0 );

			size = c_file.size;
			mode = S_IEXEC | S_IREAD;
			if( !( c_file.attrib & _A_RDONLY ) ) mode |= S_IWRITE;
			if( ( c_file.attrib & _A_SUBDIR ) ) mode |= S_IFDIR;
			mtime = c_file.time_write;
			return true;
		}
	#else
		struct dirent *dir_entry_result = NULL;
		if( readdir_r( dp, &dir_entry, &dir_entry_result ) == 0
			&& dir_entry_result != NULL )
		{
			int iDirPathLen = strlen( szDirPath );
			int iFileNameLen = strlen( dir_entry.d_name );
			if( iDirPathLen + iFileNameLen >= MAX_PATH )
				return false;

			sprintf( szFullPath, "%s%s%s", szDirPath,
				( szDirPath[ iDirPathLen - 1 ] == '/' ) ? "" : "/",
				dir_entry.d_name );
			pszName = szFullPath + strlen( szDirPath ) +
				( ( szDirPath[ iDirPathLen - 1 ] != '/' ) ? 1 : 0 );

			if( stat( szFullPath, &st ) == 0 ) {
				size = st.st_size;
				mode = st.st_mode;
				mtime = st.st_mtime;
			} else {
				size = 0;
				mode = 0;
				mtime = 0;
			}
			return true;
		}
	#endif
	return false;
}

bool CFtpServer::CEnumFileInfo::FindClose()
{
	#ifdef WIN32
		if( hFile != -1 ) _findclose( hFile );
	#else
		if( dp ) closedir( dp );
	#endif
	return true;
}
