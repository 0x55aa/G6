#include "G6.h"

static void version()
{
	printf( "G6 - TCP Transfer && Load-Balance Dispenser\n" );
	printf( "Copyright by calvin 2016\n" );
	return;
}

static void usage()
{
	printf( "USAGE : G6 -f config_pathfilename [ -t forward_thread_size ] [ -m forward_session_size ] [ --log-level (DEBUG|INFO|WARN|ERROR|FATAL) ]\n" );
	return;
}

int main( int argc , char *argv[] )
{
	struct ServerEnv	env , *penv = & env ;
	int			n ;
	
	int			nret = 0 ;
	
	/* ���ñ�׼����޻��� */
	setbuf( stdout , NULL );
	
	/* ������������� */
	srand( (unsigned)time(NULL) );
	
	/* ������־���� */
	SetLogFile( "%s/log/G6_MonitorProcess.log" , getenv("HOME") );
	SetLogLevel( LOGLEVEL_DEBUG );
	
	/* ��ʼ������������ */
	memset( penv , 0x00 , sizeof(struct ServerEnv) );
	
	/* ��ʼ�������в��� */
	penv->cmd_para.forward_thread_size = DEFAULT_FORWARD_THREAD_COUNT ;
	penv->cmd_para.forward_session_size = DEFAULT_FORWARD_SESSIONS_MAXCOUNT ;
	
	/* ���������в��� */
	if( argc == 1 )
	{
		version();
		usage();
		exit(7);
	}
	
	for( n = 1 ; n < argc ; n++ )
	{
		if( strcmp( argv[n] , "-v" ) == 0 && 1 + 1 == argc )
		{
			version();
			exit(0);
		}
		else if( strcmp( argv[n] , "-f" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.config_pathfilename = argv[n] ;
		}
		else if( strcmp( argv[n] , "-t" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_thread_size = atol(argv[n]) ;
		}
		else if( strcmp( argv[n] , "-s" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_session_size = atol(argv[n]) ;
		}
		else if( strcmp( argv[n] , "--log-level" ) == 0 && n + 1 < argc )
		{
			n++;
			if( strcmp( argv[n] , "DEBUG" ) == 0 )
				SetLogLevel( LOGLEVEL_DEBUG );
			else if( strcmp( argv[n] , "INFO" ) == 0 )
				SetLogLevel( LOGLEVEL_INFO );
			else if( strcmp( argv[n] , "WARN" ) == 0 )
				SetLogLevel( LOGLEVEL_WARN );
			else if( strcmp( argv[n] , "ERROR" ) == 0 )
				SetLogLevel( LOGLEVEL_ERROR );
			else if( strcmp( argv[n] , "FATAL" ) == 0 )
				SetLogLevel( LOGLEVEL_FATAL );
			else
			{
				fprintf( stderr , "invalid log level[%s]\r\n" , argv[n] );
				usage();
				exit(7);
			}
		}
		else
		{
			fprintf( stderr , "invalid opt[%s]\r\n" , argv[n] );
			usage();
			exit(7);
		}
	}
	
	if( penv->cmd_para.config_pathfilename == NULL )
	{
		usage();
		exit(7);
	}
	
	/* ��ʼ������ */
	penv->accept_epoll_fd = epoll_create( 1024 );
	if( penv->accept_epoll_fd == -1 )
	{
		printf( "epoll_create failed , errno[%d]" , errno );
		return 1;
	}
	
	penv->forward_thread_tid = (pthread_t *)malloc( sizeof(pthread_t) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_thread_tid == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return 1;
	}
	memset( penv->forward_thread_tid , 0x00 , sizeof(pthread_t) * penv->cmd_para.forward_thread_size );
	
	penv->accept_pipe = (struct PipeFds *)malloc( sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size ) ;
	if( penv->accept_pipe == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return 1;
	}
	memset( penv->accept_pipe , 0x00 , sizeof(struct PipeFds) * penv->cmd_para.forward_thread_size );
	
	penv->forward_epoll_fd = (int *)malloc( sizeof(int) * penv->cmd_para.forward_thread_size ) ;
	if( penv->forward_epoll_fd == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return 1;
	}
	memset( penv->forward_epoll_fd , 0x00 , sizeof(int) * penv->cmd_para.forward_thread_size );
	
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		penv->forward_epoll_fd[n] = epoll_create( 1024 );
		if( penv->forward_epoll_fd[n] == -1 )
		{
			printf( "epoll_create failed , errno[%d]" , errno );
			return 1;
		}
	}
	
	penv->forward_sessions = (struct ForwardSession *)malloc( sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size ) ;
	if( penv->forward_sessions == NULL )
	{
		printf( "malloc failed , errno[%d]" , errno );
		return 1;
	}
	memset( penv->forward_sessions , 0x00 , sizeof(struct ForwardSession) * penv->cmd_para.forward_session_size );
	
	InfoLog( __FILE__ , __LINE__ , "--- G5 beginning ---" );
	
	/* װ������ */
	nret = LoadConfig( penv ) ;
	if( nret )
	{
		printf( "LoadConfig failed[%d]\n" , nret );
		InfoLog( __FILE__ , __LINE__ , "--- G5 finished ---" );
		return -nret;
	}
	
	/* �����ظ����� */
	nret = BindDaemonServer( NULL , & _MonitorProcess , penv , NULL );
	
	/* ж������ */
	UnloadConfig( penv );
	
	/* ������ */
	close( penv->accept_epoll_fd );
	free( penv->forward_thread_tid );
	free( penv->forward_sessions );
	
	for( n = 0 ; n < penv->cmd_para.forward_thread_size ; n++ )
	{
		close( penv->forward_epoll_fd[n] );
	}
	
	InfoLog( __FILE__ , __LINE__ , "--- G5 finished ---" );
	return -nret;
}

