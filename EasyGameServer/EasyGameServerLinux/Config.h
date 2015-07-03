#pragma once


/// DB ���� ��Ʈ�� 
#define DB_CONN_STR "./database/user_example.db3"
#define LISTEN_PORT	9001

/// Ÿ�̸� ���� ���� (ms)
#define POLL_INTERVAL	100

/// �÷��̾��� HeartBeat �ֱ�
#define PLAYER_HEART_BEAT	1000

/// 1�и��� �÷��̾� ���� ����
#define PLAYER_DB_UPDATE_INTERVAL	(60*1000)

/// ���Ǻ� send/recv ���� ũ��
#define BUFSIZE	(1024*10)

/// �ִ� ���� ����
#define MAX_CONNECTION	1024