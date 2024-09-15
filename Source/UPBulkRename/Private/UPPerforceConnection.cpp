// Copyright 2024 PufStudio. All Rights Reserved.

#include "UPPerforceConnection.h"
#include "UPBulkRenameSettings.h"

DEFINE_LOG_CATEGORY(LogUPBulkRename);
#define TO_TCHAR(InText, bIsUnicodeServer) (bIsUnicodeServer ? UTF8_TO_TCHAR(InText) : ANSI_TO_TCHAR(InText))
#define FROM_TCHAR(InText, bIsUnicodeServer) (bIsUnicodeServer ? TCHAR_TO_UTF8(InText) : TCHAR_TO_ANSI(InText))

const FString FP4Record::EmptyStr;


void FUPP4ClientUser::OutputStat(StrDict* VarList)
{
	FP4Record Record;
	StrRef Var, Value;
	// Iterate over each variable and add to records
	for (int32 Index = 0; VarList->GetVar(Index, Var, Value); Index++)
	{
		Record.Add(TO_TCHAR(Var.Text(), IsUnicodeServer), TO_TCHAR(Value.Text(), IsUnicodeServer));
	}
	Records.Add(Record);
}

void FUPP4ClientUser::Message(Error* err)
{
	StrBuf Buffer;
	err->Fmt(Buffer, EF_PLAIN);

	FString Message(TO_TCHAR(Buffer.Text(), IsUnicodeServer));

	// Previously we used ::HandleError which would have \n at the end of each line.
	// For now we should add that to maintain compatibility with existing code.
	if (!Message.EndsWith(TEXT("\n")))
	{
		Message.Append(TEXT("\n"));
	}

	if (err->GetSeverity() <= ErrorSeverity::E_INFO)
	{
		ResultInfo.InfoMessages.Add(FText::FromString(MoveTemp(Message)));
	}
	else
	{
		ResultInfo.ErrorMessages.Add(FText::FromString(MoveTemp(Message)));
	}
}

void FUPP4ClientUser::OutputInfo(char Indent, const char* InInfo)
{
	ResultInfo.InfoMessages.Add(FText::FromString(FString(TO_TCHAR(InInfo, IsUnicodeServer))));
}

void FUPP4ClientUser::OutputError(const char* errBuf)
{
	ResultInfo.ErrorMessages.Add(FText::FromString(FString(TO_TCHAR(errBuf, IsUnicodeServer))));
}

void FUPP4LoginClientUser::Prompt(const StrPtr& InMessage, StrBuf& OutPrompt, int NoEcho, Error* InError)
{
	OutPrompt.Set(FROM_TCHAR(*Password, IsUnicodeServer));
}

bool FUPPerforceConnection::Init()
{
	UUPBulkRenameSettings* Settings = GetMutableDefault<UUPBulkRenameSettings>();
	// test client valid?
	Error P4Error;
	m_Client.SetProg("UE");
	m_Client.SetProtocol("tag", "");
	m_Client.SetProtocol("enableStreams", "");
	// Set user info and try to login?
	m_Client.SetPort(FROM_TCHAR(*Settings->Port, IsUnicodeServer));
	m_Client.SetUser(FROM_TCHAR(*Settings->User, IsUnicodeServer));
	m_Client.SetClient(FROM_TCHAR(*Settings->Workspace, IsUnicodeServer));
	m_Client.SetPassword(FROM_TCHAR(*Settings->Password, IsUnicodeServer));
	m_Client.Init(&P4Error);
	if (P4Error.Test())
	{
		StrBuf ErrorMessage;
		P4Error.Fmt(&ErrorMessage);

		UE_LOG(LogUPBulkRename, Error, TEXT("P4ERROR: Invalid connection to server."));
		UE_LOG(LogUPBulkRename, Error, TEXT("%s"), ANSI_TO_TCHAR(ErrorMessage.Text()));
		return false;
	}
	// get if is unicode server by p4 info
	TArray<FString> TempParams;
	if (RunCommand(TEXT("info"), TempParams))
	{
		IsUnicodeServer = m_Records[0].Find(TEXT("unicode")) != nullptr;
		if(IsUnicodeServer)
		{
			m_Client.SetTrans(CharSetApi::UTF_8);
		}
	}
	else
	{
		UE_LOG(LogUPBulkRename, Error, TEXT("P4ERROR: Invalid connection to server."));
		return false;
	}


	
	m_Records.Reset();
	FUPP4LoginClientUser User(Settings->Password, m_Records, m_ResultInfo, IsUnicodeServer);
	const char *ArgV[] = { "-a" };
	m_Client.SetArgv(1, const_cast<char**>(ArgV));
	m_Client.Run("login", &User);
	if (m_ResultInfo.HasErrors())
	{
		UE_LOG(LogUPBulkRename, Error, TEXT("Login failed"));
		for (const FText& ErrorMessage : m_ResultInfo.ErrorMessages)
		{
			UE_LOG(LogUPBulkRename, Error, TEXT("    %s"), *ErrorMessage.ToString());
		}
		return false;
	}
	return true;
}

bool FUPPerforceConnection::RunCommand(const FString& Command, const TArray<FString>& Params)
{
	// Prepare arguments
	int32 ArgC = Params.Num();
	UTF8CHAR** ArgV = new UTF8CHAR*[ArgC];
	for (int32 Index = 0; Index < ArgC; Index++)
	{
		if(IsUnicodeServer)
		{
			FTCHARToUTF8 UTF8String(*Params[Index]);
			ArgV[Index] = new UTF8CHAR[UTF8String.Length() + 1];
			FMemory::Memcpy(ArgV[Index], UTF8String.Get(), UTF8String.Length() + 1);
		}
		else
		{
			ArgV[Index] = new UTF8CHAR[Params[Index].Len() + 1];
			FMemory::Memcpy(ArgV[Index], TCHAR_TO_ANSI(*Params[Index]), Params[Index].Len() + 1);
		}
		
	}
	m_Client.SetArgv(ArgC, (char**)ArgV);
	m_Records.Reset();
	FUPP4ClientUser User(m_Records, m_ResultInfo, IsUnicodeServer);
	m_Client.Run(FROM_TCHAR(*Command, IsUnicodeServer), &User);

	// Free arguments
	for (int32 Index = 0; Index < ArgC; Index++)
	{
		delete [] ArgV[Index];
	}
	delete [] ArgV;

	// TODO: Do I need break and keep alive???
	return m_Records.Num() > 0;
}
