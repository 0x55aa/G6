#include "G6.h"

static int SelectServerAddress( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	struct ForwardRule	*p_forward_rule = p_forward_session->p_forward_rule ;
	
	if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_MS ) )
	{
		int	n ;
		for( n = 0 ; n < p_forward_rule->servers_addr_size ; n++ )
		{
			if(	p_forward_rule->servers_addr[n].server_unable == 0
				||
				(
					p_forward_rule->servers_addr[n].server_unable == 1
					&&
					p_forward_rule->servers_addr[n].timestamp_to_enable >= time(NULL)
				)
			)
			{
				p_forward_session->balance_algorithm.MS.server_index = p_forward_rule->balance_algorithm.MS.server_index ;
				p_forward_rule->balance_algorithm.MS.server_index++;
				if( p_forward_rule->balance_algorithm.MS.server_index >= p_forward_rule->servers_addr_size )
					p_forward_rule->balance_algorithm.MS.server_index = 0 ;
				break;
			}
			
			p_forward_rule->balance_algorithm.MS.server_index++;
			if( p_forward_rule->balance_algorithm.MS.server_index >= p_forward_rule->servers_addr_size )
				p_forward_rule->balance_algorithm.MS.server_index = 0 ;
		}
		if( n >= p_forward_rule->servers_addr_size )
		{
			ErrorLog( __FILE__ , __LINE__ , "all servers unable" );
			return -1;
		}
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RR ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_LC ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RT ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_RD ) )
	{
	}
	else if( STRCMP( p_forward_rule->load_balance_algorithm , == , LOAD_BALANCE_ALGORITHM_HS ) )
	{
	}
	else
	{
		ErrorLog( __FILE__ , __LINE__ , "load_balance_algorithm[%s] invalid" , p_forward_session->p_forward_rule->load_balance_algorithm );
		return -1;
	}
	
	return 0;
}

static void ResolveSocketError( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	struct ServerNetAddress		*p_servers_addr = p_forward_session->p_forward_rule->servers_addr + p_forward_session->balance_algorithm.MS.server_index ;
	
	p_servers_addr->server_unable = 1 ;
	p_servers_addr->timestamp_to_enable = time(NULL) + 600 ;
	
	return;
}

static int TryToConnectServer( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
	_SOCKLEN_T		addr_len = sizeof(struct sockaddr_in) ;
	struct epoll_event	event ;
	
	int			nret = 0 ;
	
	/* ���ݸ��ؾ����㷨ѡ������ */
	nret = SelectServerAddress( penv , p_forward_session ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "SelectServerAddress failed[%d]" , nret );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return -1;
	}
	
	/* �������ӷ���˵Ŀͻ��� */
	p_forward_session->sock = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
	if( p_forward_session->sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "socket failed , errno[%d]" , errno );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return -1;
	}
	
	SetNonBlocking( p_forward_session->sock );
	
	/* ���ӷ���� */
InfoLog( __FILE__ , __LINE__ , "p_forward_session->balance_algorithm.MS.server_index[%d]" , p_forward_session->balance_algorithm.MS.server_index );
	nret = connect( p_forward_session->sock , ( struct sockaddr *) & (p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr) , addr_len );
	if( nret == -1 )
	{
		if( _ERRNO == _EINPROGRESS || _ERRNO == _EWOULDBLOCK ) /* �������� */
		{
			p_forward_session->status = FORWARD_SESSION_STATUS_CONNECTING ;
			
			InfoLog( __FILE__ , __LINE__ , "#%d# connecting [%s:%d] ..." , p_forward_session->sock , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.ip , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.port.port_int );
			
			memset( & (event) , 0x00 , sizeof(struct epoll_event) );
			event.data.ptr = p_forward_session ;
			event.events = EPOLLOUT | EPOLLERR ;
			epoll_ctl( penv->accept_epoll_fd , EPOLL_CTL_ADD , p_forward_session->sock , & event );
		}
		else /* ����ʧ�� */
		{
			ErrorLog( __FILE__ , __LINE__ , "connect [%s:%d] failed , errno[%d]" , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.ip , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.port.port_int , errno );
			ResolveSocketError( penv , p_forward_session );
			SetForwardSessionUnused( p_forward_session );
			SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
			return -1;
		}
	}
	else /* ���ӳɹ� */
	{
		p_forward_session->status = FORWARD_SESSION_STATUS_CONNECTED ;
		
		InfoLog( __FILE__ , __LINE__ , "connect [%s:%d] ok" , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.ip , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.port.port_int );
		
		if( p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].server_unable == 1 )
			p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].server_unable = 0 ;
		
		memset( & (event) , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->forward_epoll_fd_array[(p_forward_session->sock)%(penv->cmd_para.forward_thread_size)] , EPOLL_CTL_MOD , p_forward_session->sock , & event );
		
		memset( & (event) , 0x00 , sizeof(struct epoll_event) );
		event.data.ptr = p_forward_session->p_reverse_forward_session ;
		event.events = EPOLLIN | EPOLLERR ;
		epoll_ctl( penv->forward_epoll_fd_array[(p_forward_session->sock)%(penv->cmd_para.forward_thread_size)] , EPOLL_CTL_MOD , p_forward_session->p_reverse_forward_session->sock , & event );
	}
	
	return 0;
}

static int OnListenAccept( struct ServerEnv *penv , struct ForwardSession *p_listen_session )
{
	struct ForwardSession	*p_forward_session = NULL ;
	struct ForwardSession	*p_reverse_forward_session = NULL ;
	_SOCKLEN_T		addr_len = sizeof(struct sockaddr_in) ;
	
	int			nret = 0 ;
	
	p_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed" );
		return -1;
	}
	
	p_reverse_forward_session = GetForwardSessionUnused( penv ) ;
	if( p_reverse_forward_session == NULL )
	{
		ErrorLog( __FILE__ , __LINE__ , "GetForwardSessionUnused failed" );
		return -1;
	}
	
	p_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_forward_session->p_reverse_forward_session = p_reverse_forward_session ;
	
	p_reverse_forward_session->p_forward_rule = p_listen_session->p_forward_rule ;
	p_reverse_forward_session->p_reverse_forward_session = p_forward_session ;
	
	p_forward_session->sock = accept( p_listen_session->sock , (struct sockaddr *) & (p_forward_session->netaddr) , & addr_len ) ;
	if( p_forward_session->sock == -1 )
	{
		ErrorLog( __FILE__ , __LINE__ , "accept failed , errno[%d]" , errno );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_reverse_forward_session );
		return -1;
	}
	else
	{
		GetNetAddress( & (p_forward_session->netaddr) );
		DebugLog( __FILE__ , __LINE__ , "accept #%d# from [%s:%d] ok" , p_forward_session->sock , p_forward_session->netaddr.ip , p_forward_session->netaddr.port.port_int );
	}
	
	SetNonBlocking( p_forward_session->sock );
	
	nret = TryToConnectServer( penv , p_reverse_forward_session ) ;
	if( nret )
	{
		ErrorLog( __FILE__ , __LINE__ , "TryToConnectServer failed[%d]" , nret );
		SetForwardSessionUnused( p_forward_session );
		return -1;
	}
	
	return 0;
}

static int OnConnectingServer( struct ServerEnv *penv , struct ForwardSession *p_forward_session )
{
#if ( defined __linux ) || ( defined __unix )
	int			error , code ;
#endif
	_SOCKLEN_T		addr_len ;
	struct epoll_event	event ;
	
	/* ���Ƕ������ӽ�� */
#if ( defined __linux ) || ( defined __unix )
	addr_len = sizeof(int) ;
	code = getsockopt( p_forward_session->sock , SOL_SOCKET , SO_ERROR , & error , & addr_len ) ;
	if( code < 0 || error )
	{
		ResolveSocketError( penv , p_forward_session );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return -1;
	}
#elif ( defined _WIN32 )
	addr_len = sizeof(struct sockaddr_in) ;
	nret = connect( p_forward_session->sock , ( struct sockaddr *) & (p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index]->netaddr) , addr_len ) ;
	if( nret == -1 && _ERRNO == _EISCONN )
	{
		;
	}
	else
        {
		ResolveSocketError( penv , p_forward_session );
		SetForwardSessionUnused( p_forward_session );
		SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
		return -1;
        }
#endif
	
	/* ���ӳɹ� */
	p_forward_session->status = FORWARD_SESSION_STATUS_CONNECTED ;
	
	InfoLog( __FILE__ , __LINE__ , "connect2 [%s:%d] ok" , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.ip , p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].netaddr.port.port_int );
	
	if( p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].server_unable == 1 )
		p_forward_session->p_forward_rule->servers_addr[p_forward_session->balance_algorithm.MS.server_index].server_unable = 0 ;
	
	memset( & (event) , 0x00 , sizeof(struct epoll_event) );
	event.data.ptr = p_forward_session ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->forward_epoll_fd_array[(p_forward_session->sock)%(penv->cmd_para.forward_thread_size)] , EPOLL_CTL_ADD , p_forward_session->sock , & event );
	
	memset( & (event) , 0x00 , sizeof(struct epoll_event) );
	event.data.ptr = p_forward_session->p_reverse_forward_session ;
	event.events = EPOLLIN | EPOLLERR ;
	epoll_ctl( penv->forward_epoll_fd_array[(p_forward_session->sock)%(penv->cmd_para.forward_thread_size)] , EPOLL_CTL_ADD , p_forward_session->p_reverse_forward_session->sock , & event );
	
	return 0;
}

static void *AcceptThread( struct ServerEnv *penv )
{
	struct epoll_event	events[ WAIT_EVENTS_COUNT ] ;
	int			event_count ;
	int			event_index ;
	struct epoll_event	*p_event = NULL ;
	
	char			command ;
	
	struct ForwardSession	*p_forward_session = NULL ;
	
	int			nret = 0 ;
	
	/* ������־���� */
	SetLogFile( "%s/log/G6_WorkerProcess_AcceptThread.log" , getenv("HOME") );
	SetLogLevel( penv->log_level );
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.AcceptThread bebin ---" );
	
	/* ������ѭ�� */
	event_count = epoll_wait( penv->accept_epoll_fd , events , sizeof(events)/sizeof(events[0]) , -1 ) ;
	for( event_index = 0 , p_event = events ; event_index < event_count ; event_index++ , p_event++ )
	{
		if( p_event->data.ptr == NULL ) /* ����ܵ��¼� */
		{
			if( p_event->events & EPOLLIN )
			{
				read( penv->accept_pipe.fds[0] , & command , 1 );
				InfoLog( __FILE__ , __LINE__ , "read command pipe[%c]" , command );
			}
			else if( p_event->events & EPOLLERR )
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLERR" );
			}
			else if( p_event->events & EPOLLHUP )
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe EPOLLHUP" );
			}
			else
			{
				ErrorLog( __FILE__ , __LINE__ , "command pipe [%d]" , p_event->events );
			}
		}
		else
		{
			p_forward_session = p_event->data.ptr ;
			
			if( p_forward_session->status == FORWARD_SESSION_STATUS_LISTEN )
			{
				if( p_event->events & EPOLLIN ) /* �����˿��¼� */
				{
					nret = OnListenAccept( penv , p_forward_session ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnListenAccept failed[%d]" , nret );
					}
					else
					{
						DebugLog( __FILE__ , __LINE__ , "OnListenAccept ok" );
					}
				}
				else if( p_event->events & EPOLLERR )
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLERR" );
					exit(0);
				}
				else if( p_event->events & EPOLLHUP )
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLHUP" );
					exit(0);
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe [%d]" , p_event->events );
					exit(0);
				}
			}
			else if( p_forward_session->status == FORWARD_SESSION_STATUS_CONNECTING )
			{
				if( p_event->events & EPOLLOUT ) /* �Ƕ��������¼� */
				{
					nret = OnConnectingServer( penv , p_forward_session ) ;
					if( nret )
					{
						ErrorLog( __FILE__ , __LINE__ , "OnConnectingServer failed[%d]" , nret );
					}
					else
					{
						DebugLog( __FILE__ , __LINE__ , "OnConnectingServer ok" );
					}
				}
				else if( p_event->events & EPOLLERR )
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLERR" );
					ResolveSocketError( penv , p_forward_session );
					SetForwardSessionUnused( p_forward_session );
					SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
				}
				else if( p_event->events & EPOLLHUP )
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe EPOLLHUP" );
					ResolveSocketError( penv , p_forward_session );
					SetForwardSessionUnused( p_forward_session );
					SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
				}
				else
				{
					ErrorLog( __FILE__ , __LINE__ , "accept pipe [%d]" , p_event->events );
					ResolveSocketError( penv , p_forward_session );
					SetForwardSessionUnused( p_forward_session );
					SetForwardSessionUnused( p_forward_session->p_reverse_forward_session );
				}
			}
		}
	}
	
	InfoLog( __FILE__ , __LINE__ , "--- G6.WorkerProcess.AcceptThread finish ---" );
	
	return 0;
}

void *_AcceptThread( void *pv )
{
	return AcceptThread( (struct ServerEnv *)pv );
}

