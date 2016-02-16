#include "G6.h"

static void version()
{
	printf( "G6 - TCP Transfer && Load-Balance Dispenser\n" );
	printf( "Copyright by calvin 2016\n" );
	return;
}

static void usage()
{
	printf( "USAGE : G6 -f config_pathfilename [ -t forward_thread_count ] [ -m session_max_count ] [ --log-level (DEBUG|INFO|WARN|ERROR|FATAL) ]\n" );
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
	penv->cmd_para.forward_thread_count = 2 ;
	penv->cmd_para.forward_session_maxcount = DEFAULT_FORWARD_SESSIONS_MAXCOUNT ;
	
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
			penv->cmd_para.forward_thread_count = atol(argv[n]) ;
		}
		else if( strcmp( argv[n] , "-s" ) == 0 && n + 1 < argc )
		{
			n++;
			penv->cmd_para.forward_session_maxcount = atol(argv[n]) ;
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
	
	InfoLog( __FILE__ , __LINE__ , "--- G5 finished ---" );
	return -nret;
}

