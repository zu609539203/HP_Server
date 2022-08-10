#ifndef _MessageHeader_hpp
#define _MessageHeader_hpp

enum CMD {
	CMD_NetMsg_Login,
	CMD_NetMsg_LoginR,
	CMD_NetMsg_Logout,
	CMD_NetMsg_LogoutR,
	CMD_NEW_USER_IN,
	CMD_ERROR,
	CMD_HEART_C2S,
	CMD_HEART_S2C
};
struct NetMsg_DataHeader
{
	NetMsg_DataHeader()
	{
		dataLength = sizeof(NetMsg_DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;
	short cmd;
};

// DataPackage
struct NetMsg_Login : public NetMsg_DataHeader
{
	NetMsg_Login()
	{
		dataLength = sizeof(NetMsg_Login);
		cmd = CMD_NetMsg_Login;
	}
	char userName[32];
	char passWord[32];
	char message[32];
};
struct NetMsg_LoginR : public NetMsg_DataHeader
{
	NetMsg_LoginR()
	{
		dataLength = sizeof(NetMsg_LoginR);
		cmd = CMD_NetMsg_LoginR;
		result = 0;
	}
	int result;
	char message[92];
};
struct NetMsg_Logout : public NetMsg_DataHeader
{
	NetMsg_Logout()
	{
		dataLength = sizeof(NetMsg_Logout);
		cmd = CMD_NetMsg_Logout;
	}
	char userName[32];
};
struct NetMsg_LogoutR : public NetMsg_DataHeader
{
	NetMsg_LogoutR()
	{
		dataLength = sizeof(NetMsg_LogoutR);
		cmd = CMD_NetMsg_LoginR;
		result = 0;
	}
	int result;
};
struct NewUserIn : public NetMsg_DataHeader
{
	NewUserIn()
	{
		dataLength = sizeof(NewUserIn);
		cmd = CMD_NEW_USER_IN;
		sock = 0;
	}
	int sock;
};
struct NetMsg_Heart_C2S : public NetMsg_DataHeader
{
	NetMsg_Heart_C2S()
	{
		dataLength = sizeof(NetMsg_Heart_C2S);
		cmd = CMD_HEART_C2S;
	}
};
struct NetMsg_Heart_S2C : public NetMsg_DataHeader
{
	NetMsg_Heart_S2C()
	{
		dataLength = sizeof(NetMsg_Heart_S2C);
		cmd = CMD_HEART_S2C;
	}
};
#endif // !_MessageHeader_hpp

