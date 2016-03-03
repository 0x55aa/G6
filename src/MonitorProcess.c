#include "G6.h"

int			g_exit_flag = 0 ;

static void sig_proc( int sig_no )
{
	if( sig_no == SIGUSR2 )
	{
		pid_t			pid ;
		char			command ;
		
		int			nret = 0 ;
		
		nret = SaveListenSockets( g_penv ) ;
		if( nret )
		{
			ErrorLog( __FILE__ , __LINE__ , "SaveListenSockets faild[%d]" , nret );
			return;
		}
		
		DebugLog( __FILE__ , __LINE__ , "write request pipe Q ..." );
		write( g_penv->request_pipe.fds[1] , "Q" , 1 );
		DebugLog( __FILE__ , __LINE__ , "write request pipe Q ok" );
		
		DebugLog( __FILE__ , __LINE__ , "read response pipe ..." );
		read( g_penv->response_pipe.fds[0] , & command , 1 );
		DebugLog( __FILE__ , __LINE__ , "read response pipe ok %c" , command );
		
		pid = fork() ;
		if( pid == -1 )
		{
			;
		}
		else if( pid == 0 )
		{
			//CleanEnvirment( g_penv );
			
			execvp( "G6" , g_penv->argv );
			
			exit(9);
		}
		
		g_exit_flag = 1 ;
		DebugLog( __FILE__ , __LINE__ , "set g_exit_flag[%d]" , g_exit_flag );
	}
	else if( sig_no == SIGTERM )
	{
		char			command ;
		
		DebugLog( __FILE__ , __LINE__ , "write request pipe Q ..." );
		write( g_penv->request_pipe.fds[1] , "Q" , 1 );
		DebugLog( __FILE__ , __LINE__ , "write request pipe Q ok" );
		
		DebugLog( __FILE__ , __LINE__ , "read response pipe ..." );
		read( g_penv->request_pipe.fds[0] , & command , 1 );
		DebugLog( __FILE__ , __LINE__ , "read response pipe ok %c" , command );
		
		g_exit_flag = 1 ;
		DebugLog( __FILE__ , __LINE__ , "set g_exit_flag[%d]" , g_exit_flag );
	}
	
	return;
}

int MonitorProcess( struct ServerEnv *penv )
{
	struct epoll_event	event ;
	
	pid_t			pid ;
	int			status ;
	
	int			nret = 0 ;
	
	/* ������־����ļ� */
	InfoLog( __FILE__ , __LINE__ , "--- G6.MonitorProcess ---" );
	
	/* �����źž�� */
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGPIPE , SIG_IGN );
	g_penv = penv ;
	signal( SIGTERM , sig_proc );
	signal( SIGUSR2 , sig_proc );
	
	/* ������ѭ�� */
	while( g_exit_flag == 0 )
	{
		sleep(1);
		
		/* �����ܵ� */
		_PIPE1 :
		nret = pipe( penv->request_pipe.fds ) ;
		if( nret == -1 )
		{
			if( errno == EINTR )
				goto _PIPE1;
			
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			return -1;
		}
		SetCloseExec2( penv->request_pipe.fds[0] , penv->request_pipe.fds[1] );
		
		_PIPE2 :
		nret = pipe( penv->response_pipe.fds ) ;
		if( nret == -1 )
		{
			if( errno == EINTR )
				goto _PIPE2;
			
			ErrorLog( __FILE__ , __LINE__ , "pipe failed , errno[%d]" , errno );
			close( penv->request_pipe.fds[0] );
			close( penv->request_pipe.fds[1] );
			return -1;
		}
		SetCloseExec4( penv->request_pipe.fds[0] , penv->request_pipe.fds[1] , penv->response_pipe.fds[0] , penv->response_pipe.fds[1] );
		
		/* ���ܵ����˼��뵽�����˿�epoll�� */
		memset( & event , 0x00 , sizeof(event) );
		event.data.ptr = NULL ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , penv->request_pipe.fds[0] , & event );
		
		/* �����ӽ��� */
		_FORK :
		penv->pid = fork() ;
		if( penv->pid == -1 )
		{
			if( errno == EINTR )
				goto _FORK;
			
			ErrorLog( __FILE__ , __LINE__ , "fork failed , errno[%d]" , errno );
			close( penv->request_pipe.fds[0] );
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[0] );
			close( penv->response_pipe.fds[1] );
			return -1;
		}
		else if( penv->pid == 0 )
		{
			InfoLog( __FILE__ , __LINE__ , "child : [%ld]fork[%ld]" , getppid() , getpid() );
			
			close( penv->response_pipe.fds[0] );
			
			nret = WorkerProcess( penv ) ;
			
			close( penv->request_pipe.fds[0] );
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[1] );
			
			exit(-nret);
		}
		else
		{
			InfoLog( __FILE__ , __LINE__ , "parent : [%ld]fork[%ld]" , getpid() , penv->pid );
			
			close( penv->request_pipe.fds[0] );
			close( penv->response_pipe.fds[1] );
		}
		
		/* ����ӽ��̽��� */
		_WAITPID :
		pid = waitpid( -1 , & status , 0 );
		if( pid == -1 )
		{
			if( errno == EINTR )
				goto _WAITPID;
			
			ErrorLog( __FILE__ , __LINE__ , "waitpid failed , errno[%d]" , errno );
			close( penv->request_pipe.fds[1] );
			close( penv->response_pipe.fds[0] );
			return -1;
		}
		if( pid != penv->pid )
			goto _WAITPID;
		
		close( penv->request_pipe.fds[1] );
		close( penv->response_pipe.fds[0] );
		
		InfoLog( __FILE__ , __LINE__
			, "waitpid[%ld] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d] WCOREDUMP[%d]"
			, penv->pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) , WCOREDUMP(status) );
	}
	
	return 0;
}

int _MonitorProcess( void *pv )
{
	return MonitorProcess( (struct ServerEnv *)pv );
}

