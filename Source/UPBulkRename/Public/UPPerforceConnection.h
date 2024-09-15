// Copyright 2024 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include <p4/clientapi.h>
#include <p4/i18napi.h>
THIRD_PARTY_INCLUDES_END

DECLARE_LOG_CATEGORY_EXTERN(LogUPBulkRename, Warning, All)
/**
 * simplified perforce required structs and class def
 * copy unreal implementation and remove parts where 100% useless in this case.
 */

/** A map containing result of running Perforce command */
class FP4Record : public TMap<FString, FString>
{
public:
	virtual ~FP4Record() {}

	const FString& operator()(const FString& Key) const
	{
		if (const FString* Found = Find(Key))
		{
			return *Found;
		}
		return EmptyStr;
	}

	const FString& operator()(const TCHAR* Key) const
	{
		if (const FString* Found = FindByHash(FCrc::Strihash_DEPRECATED(Key), Key))
		{
			return *Found;
		}
		return EmptyStr;
	}

private:
	static const FString EmptyStr;
};
typedef TArray<FP4Record> FP4RecordSet;

/** Accumulated error and info messages for a source control operation.  */
struct FP4ResultInfo
{
	/** Append any messages from another FSourceControlResultInfo, ensuring to keep any already accumulated info. */
	void Append(const FP4ResultInfo& InResultInfo)
	{
		InfoMessages.Append(InResultInfo.InfoMessages);
		ErrorMessages.Append(InResultInfo.ErrorMessages);
		Tags.Append(InResultInfo.Tags);
	}

	bool HasErrors() const
	{
		return !ErrorMessages.IsEmpty();
	}

	/** Info and/or warning message storage */
	TArray<FText> InfoMessages;

	/** Potential error message storage */
	TArray<FText> ErrorMessages;

	/** Additional arbitrary information attached to the command */
	TArray<FString> Tags;

};

/** Custom ClientUser class for handling results and errors from Perforce commands */
class FUPP4ClientUser : public ClientUser
{
public:

	FUPP4ClientUser(FP4RecordSet& InRecords, FP4ResultInfo& InResultInfo, bool InIsUnicodeServer)
		: ClientUser()
		, Records(InRecords)
		, ResultInfo(InResultInfo)
		, IsUnicodeServer(InIsUnicodeServer)
	{}

	/**  Called by P4API when the results from running a command are ready. */
	virtual void OutputStat(StrDict* VarList) override;
	
	virtual void Message(Error* err) override;
	
	virtual void OutputInfo(char Indent, const char* InInfo) override;
	
	virtual void OutputError(const char* errBuf) override;
	
protected:	
	FP4RecordSet& Records;
	FP4ResultInfo& ResultInfo;
	bool IsUnicodeServer = false;
};

/** Custom ClientUser class for handling login commands */
class FUPP4LoginClientUser : public FUPP4ClientUser
{
public:
	FUPP4LoginClientUser(const FString& InPassword, FP4RecordSet& InRecords, FP4ResultInfo& OutResultInfo, bool InIsUnicodeServer)
		:	FUPP4ClientUser(InRecords, OutResultInfo, InIsUnicodeServer)
		,	Password(InPassword)
	{
	}

	/** Called when we are prompted for a password */
	virtual void Prompt( const StrPtr& InMessage, StrBuf& OutPrompt, int NoEcho, Error* InError ) override;

	/** Password to use when logging in */
	FString Password;
};

class FUPPerforceConnection
{
public:
	FUPPerforceConnection(ClientApi& InApi, FP4RecordSet& InRecords, FP4ResultInfo& OutResultInfo)
	: m_Client(InApi), m_Records(InRecords), m_ResultInfo(OutResultInfo)
	{
	}
	bool Init();
	bool RunCommand(const FString& Command, const TArray<FString>& Params);
	
private:
	ClientApi& m_Client;
	FP4RecordSet& m_Records;
	FP4ResultInfo& m_ResultInfo;
	bool IsUnicodeServer = false;
	
};
