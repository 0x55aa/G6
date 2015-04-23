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
	SetLogFile( "%s/log/G6.log" , getenv("HOME") );
	penv->log_level = LOGLEVEL_INFO ;
	SetLogLevel( penv->log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G5 begin ---" );
	
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
				penv->log_level = LOGLEVEL_DEBUG ;
			else if( strcmp( argv[n] , "INFO" ) == 0 )
				penv->log_level = LOGLEVEL_INFO ;
			else if( strcmp( argv[n] , "WARN" ) == 0 )
				penv->log_level = LOGLEVEL_WARN ;
			else if( strcmp( argv[n] , "ERROR" ) == 0 )
				penv->log_level = LOGLEVEL_ERROR ;
			else if( strcmp( argv[n] , "FATAL" ) == 0 )
				penv->log_level = LOGLEVEL_FATAL ;
			else
			{
				fprintf( stderr , "invalid log level[%s]\r\n" , argv[n] );
				usage();
				exit(7);
			}
			
			SetLogLevel( penv->log_level );
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
DebugLog( __FILE__ , __LINE__ , "penv->cmd_para.forward_thread_size[%ld]" , penv->cmd_para.forward_thread_size );
	
	/* ��ʼ������ */
	nret = InitEnvirment( penv ) ;
	if( nret )
	{
		printf( "InitEnvirment failed[%d]\n" , nret );
		return -nret;
	}
	
	/* װ������ */
	nret = LoadConfig( penv ) ;
	if( nret )
	{
		printf( "LoadConfig failed[%d]\n" , nret );
		return -nret;
	}
	
	/* �����ظ����� */
	nret = BindDaemonServer( NULL , & _MonitorProcess , penv , NULL );
	
	/* ж������ */
	UnloadConfig( penv );
	
	/* ������ */
	CleanEnvirment( penv );
	
	InfoLog( __FILE__ , __LINE__ , "--- G5 finish ---" );
	
	return -nret;
}

