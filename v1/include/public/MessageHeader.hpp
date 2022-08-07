// 定义结构体消息类型
// 目前服务端收发消息包大小均为 100B

#ifndef _MessageHeader_hpp
#define _MessageHeader_hpp

enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_IN,
	CMD_ERROR
};
struct DataHeader
{
	DataHeader()
	{
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;
	short cmd;
};
// DataPackage
struct Login : public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
	char message[32];
};
struct LoginResult : public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char message[92];
};
struct Logout : public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult : public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct NewUserIn : public DataHeader
{
	NewUserIn()
	{
		dataLength = sizeof(NewUserIn);
		cmd = CMD_NEW_USER_IN;
		sock = 0;
	}
	int sock;
};

#endif // !_MessageHeader_hpp