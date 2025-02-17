//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//

#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#include <unknwn.h>
#include "guid.h"
#include "CTollgateCredential.h"

#include "CUSBAuth.h"
#include "CPatternAuth.h"
#include "CFaceAuth.h"
#include "CFingerprintAuth.h"
#include "CQRAuth.h"
#include "RestClient.h"


CTollgateCredential::CTollgateCredential() :
	_cRef(1),
	_pCredProvCredentialEvents(nullptr),
	_pszUserSid(nullptr),
	_pszQualifiedUserName(nullptr),
	_fIsLocalUser(false),
	_fChecked(false),
	_fShowControls(false),
	_dwComboIndex(0)
{
	DllAddRef();

	ZeroMemory(_rgCredProvFieldDescriptors, sizeof(_rgCredProvFieldDescriptors));
	ZeroMemory(_rgFieldStatePairs, sizeof(_rgFieldStatePairs));
	ZeroMemory(_rgFieldStrings, sizeof(_rgFieldStrings));
}

CTollgateCredential::~CTollgateCredential()
{
	if (_rgFieldStrings[SFI_PASSWORD_INPUT])
	{
		size_t lenPassword = wcslen(_rgFieldStrings[SFI_PASSWORD_INPUT]);
		SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD_INPUT], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD_INPUT]));
	}
	for (int i = 0; i < ARRAYSIZE(_rgFieldStrings); i++)
	{
		CoTaskMemFree(_rgFieldStrings[i]);
		CoTaskMemFree(_rgCredProvFieldDescriptors[i].pszLabel);
	}
	CoTaskMemFree(_pszUserSid);
	CoTaskMemFree(_pszQualifiedUserName);
	DllRelease();
}


// Initializes one credential with the field information passed in.
// Set the value of the SFI_LARGE_TEXT field to pwzUsername.
HRESULT CTollgateCredential::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
	_In_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR const* rgcpfd,
	_In_ FIELD_STATE_PAIR const* rgfsp,
	_In_ ICredentialProviderUser* pcpUser)
{
	HRESULT hr = S_OK;
	_cpus = cpus;

	GUID guidProvider;
	pcpUser->GetProviderID(&guidProvider);
	_fIsLocalUser = (guidProvider == Identity_LocalUserProvider);

	// Copy the field descriptors for each field. This is useful if you want to vary the field
	// descriptors based on what Usage scenario the credential was created for.
	for (DWORD i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
	{
		_rgFieldStatePairs[i] = rgfsp[i];
		hr = FieldDescriptorCopy(rgcpfd[i], &_rgCredProvFieldDescriptors[i]);
	}

	// Initialize the String value of all the fields.
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Tollgate MFA", &_rgFieldStrings[SFI_LABEL]);
	}

	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"서버와 연결 중입니다..", &_rgFieldStrings[SFI_STAGE_STATUS]);
	}

	/*
	 *  USB 인증 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"버튼을 누른 후 USB를 삽입하십시오", &_rgFieldStrings[SFI_USB_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"USB 인식 시작", &_rgFieldStrings[SFI_USB_VERIFY]);
	}

	/*
	 *  패턴 인증 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"버튼을 눌러 패턴 인증을 요청하십시오", &_rgFieldStrings[SFI_PATTERN_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Android로 패턴 인증 요청", &_rgFieldStrings[SFI_PATTERN_REQUEST]);
	}

	/*
	 *  QR 인증 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"버튼을 눌러 QR 코드를 생성하십시오", &_rgFieldStrings[SFI_QR_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"QR 코드 발급", &_rgFieldStrings[SFI_QR_REQUEST]);
	}

	/*
	 *  지문 인증 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"버튼을 눌러 지문 인증을 요청하십시오", &_rgFieldStrings[SFI_FINGERPRINT_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Android로 지문 인증 요청", &_rgFieldStrings[SFI_FINGERPRINT_REQUEST]);
	}

	/*
	 *  안면 인증 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"버튼을 눌러 안면 인증을 요청하십시오", &_rgFieldStrings[SFI_FACE_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Android로 안면 인증 요청", &_rgFieldStrings[SFI_FACE_REQUEST]);
	}

	/*
	 *  패스워드 인증 및 OTP 관련
	 */
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"비밀번호와 OTP 값을 입력하십시오", &_rgFieldStrings[SFI_OTP_MESSAGE]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"", &_rgFieldStrings[SFI_PASSWORD_INPUT]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"Submit", &_rgFieldStrings[SFI_PASSWORD_VERIFY]);
	}
	if (SUCCEEDED(hr))
	{
		hr = SHStrDupW(L"", &_rgFieldStrings[SFI_OTP_INPUT]);
	}


	if (SUCCEEDED(hr))
	{
		hr = pcpUser->GetStringValue(PKEY_Identity_QualifiedUserName, &_pszQualifiedUserName);
	}
	if (SUCCEEDED(hr))
	{
		hr = pcpUser->GetSid(&_pszUserSid);
	}


	// 플래그 초기화
	_pUSBAuth = new CUSBAuth();
	_pPatternAuth = new CPatternAuth();
	_pFaceAuth = new CFaceAuth();
	_pFingerprintAuth = new CFingerprintAuth();
	_pQRAuth = new CQRAuth();

	return hr;
}

// LogonUI calls this in order to give us a callback in case we need to notify it of anything.
HRESULT CTollgateCredential::Advise(_In_ ICredentialProviderCredentialEvents* pcpce)
{
	if (_pCredProvCredentialEvents != nullptr)
	{
		_pCredProvCredentialEvents->Release();
	}

	HRESULT hr = pcpce->QueryInterface(IID_PPV_ARGS(&_pCredProvCredentialEvents));

	if (SUCCEEDED(hr))
	{
		PWSTR token = NULL;
		PWSTR context = NULL;

		// SID의 기타 정보 제외
		token = wcstok_s(_pszUserSid, L"-", &context);
		for (int i = 0; i < 3; i++)
		{
			token = wcstok_s(NULL, L"-", &context);
		}


		// SID에서 시스템 고유 식별값 추출 및 세팅
		token = wcstok_s(NULL, L"-", &context);
		wcscat_s(wszSystemIdentifier, 50, token);
		wcscat_s(wszSystemIdentifier, 50, L"-");

		token = wcstok_s(NULL, L"-", &context);
		wcscat_s(wszSystemIdentifier, 50, token);
		wcscat_s(wszSystemIdentifier, 50, L"-");

		token = wcstok_s(NULL, L"-", &context);
		wcscat_s(wszSystemIdentifier, 50, token);


		// 서버로부터 해당 PC와 매핑된 사용자의 factor flag를 얻어옴
		RestClient* rc = new RestClient();
		if (rc->GetUserBySystemIdentifier(wszSystemIdentifier))
		{
			// --------------- 인증 서버로부터 검증 결과 값 비교하여 인증 성공 여부 판단 ---------------
			wchar_t wcMessageFromServer[2048] = { 0, };
			DWORD retCode = rc->GetRestClientExitCode();

			switch (retCode)
			{
				// 서버와 연결 성공
			case rc->RESULT_CONNECTION_SUCCESS:
			{
				rc->GetRestClientMessage(wcMessageFromServer, 2048);
				wcscpy_s(wszUserName, wcMessageFromServer);
				break;
			}
			case rc->RESULT_CONNECTION_FAILED:
			{
				SetAuthMessage(SFI_STAGE_STATUS, L"서버와 연결에 실패하였습니다.\r\n관리자에게 문의하십시오");
				_bAuthFactorFlag = AUTH_FACTOR_INVALID;
				delete rc;
				return hr;
			}
			default:
				wcscpy_s(wszUserName, L"");
				break;
			}
		}
		else
		{
			wcscpy_s(wszUserName, L"");
		}

		// 서버로부터 매핑된 사용자명을 얻어왔을 경우 이를 이용하여 Factor flag를 얻어옴
		if (wcscmp(wszUserName, L""))
		{
			if (rc->GetAuthFactorByUser(wszUserName))
			{
				// --------------- 인증 서버로부터 검증 결과 값 비교하여 인증 성공 여부 판단 ---------------
				wchar_t wcMessageFromServer[2048] = { 0, };
				DWORD dwRetCode = rc->GetRestClientExitCode();

				switch (dwRetCode)
				{
					// 서버와 연결 성공
				case rc->RESULT_CONNECTION_SUCCESS:
				{
					rc->GetRestClientMessage(wcMessageFromServer, 2048);
					BYTE flag = (BYTE)_wtoi(&(wcMessageFromServer[0]));
					_bAuthFactorFlag = flag;

					InitializeAuthStage();
					break;
				}
				default:
					SetAuthMessage(SFI_STAGE_STATUS, L"서버와 연결 중 오류가 발생하였습니다");
					_bAuthFactorFlag = AUTH_FACTOR_INVALID;
					break;
				}
			}
			else
			{
				SetAuthMessage(SFI_STAGE_STATUS, L"인증 정보를 불러올 수 없습니다");
				_bAuthFactorFlag = AUTH_FACTOR_INVALID;
			}
		}
		else
		{
			SetAuthMessage(SFI_STAGE_STATUS, L"해당 PC와 연동된 사용자를 불러올 수 없습니다");
			_bAuthFactorFlag = AUTH_FACTOR_INVALID;
		}

		delete rc;

		// 모든 버튼 활성화
		EnableAuthStartButton(SFI_USB_VERIFY, TRUE);
		EnableAuthStartButton(SFI_PATTERN_REQUEST, TRUE);
		EnableAuthStartButton(SFI_QR_REQUEST, TRUE);
		EnableAuthStartButton(SFI_FINGERPRINT_REQUEST, TRUE);
		EnableAuthStartButton(SFI_FACE_REQUEST, TRUE);
	}

	return hr;
}

// LogonUI calls this to tell us to release the callback.
HRESULT CTollgateCredential::UnAdvise()
{
	if (_pCredProvCredentialEvents)
	{
		_pCredProvCredentialEvents->Release();
	}
	_pCredProvCredentialEvents = nullptr;

	return S_OK;
}

// LogonUI calls this function when our tile is selected (zoomed)
// If you simply want fields to show/hide based on the selected state,
// there's no need to do anything here - you can set that up in the
// field definitions. But if you want to do something
// more complicated, like change the contents of a field when the tile is
// selected, you would do it here.
HRESULT CTollgateCredential::SetSelected(_Out_ BOOL* pbAutoLogon)
{
	*pbAutoLogon = FALSE;
	return S_OK;
}

// Similarly to SetSelected, LogonUI calls this when your tile was selected
// and now no longer is. The most common thing to do here (which we do below)
// is to clear out the password field.
HRESULT CTollgateCredential::SetDeselected()
{
	HRESULT hr = S_OK;
	if (_rgFieldStrings[SFI_PASSWORD_INPUT])
	{
		size_t lenPassword = wcslen(_rgFieldStrings[SFI_PASSWORD_INPUT]);
		SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD_INPUT], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD_INPUT]));

		CoTaskMemFree(_rgFieldStrings[SFI_PASSWORD_INPUT]);
		hr = SHStrDupW(L"", &_rgFieldStrings[SFI_PASSWORD_INPUT]);

		if (SUCCEEDED(hr) && _pCredProvCredentialEvents)
		{
			_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD_INPUT, _rgFieldStrings[SFI_PASSWORD_INPUT]);
		}
	}

	return hr;
}

// Get info for a particular field of a tile. Called by logonUI to get information
// to display the tile.
HRESULT CTollgateCredential::GetFieldState(DWORD dwFieldID,
	_Out_ CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
	_Out_ CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis)
{
	HRESULT hr;

	// Validate our parameters.
	if ((dwFieldID < ARRAYSIZE(_rgFieldStatePairs)))
	{
		*pcpfs = _rgFieldStatePairs[dwFieldID].cpfs;
		*pcpfis = _rgFieldStatePairs[dwFieldID].cpfis;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}
	return hr;
}

// Sets ppwsz to the string value of the field at the index dwFieldID
HRESULT CTollgateCredential::GetStringValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ PWSTR* ppwsz)
{
	HRESULT hr;
	*ppwsz = nullptr;

	// Check to make sure dwFieldID is a legitimate index
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors))
	{
		// Make a copy of the string and return that. The caller
		// is responsible for freeing it.
		hr = SHStrDupW(_rgFieldStrings[dwFieldID], ppwsz);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Get the image to show in the user tile
HRESULT CTollgateCredential::GetBitmapValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ HBITMAP* phbmp)
{
	HRESULT hr;
	*phbmp = nullptr;

	if ((SFI_TILEIMAGE == dwFieldID))
	{
		HBITMAP hbmp = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_TILE_IMAGE));
		if (hbmp != nullptr)
		{
			hr = S_OK;
			*phbmp = hbmp;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Sets pdwAdjacentTo to the index of the field the submit button should be
// adjacent to. We recommend that the submit button is placed next to the last
// field which the user is required to enter information in. Optional fields
// should be below the submit button.
HRESULT CTollgateCredential::GetSubmitButtonValue(DWORD dwFieldID, _Out_ DWORD* pdwAdjacentTo)
{
	HRESULT hr;

	if (SFI_PASSWORD_VERIFY == dwFieldID)
	{
		// pdwAdjacentTo is a pointer to the fieldID you want the submit button to
		// appear next to.
		*pdwAdjacentTo = SFI_PASSWORD_INPUT;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Sets the value of a field which can accept a string as a value.
// This is called on each keystroke when a user types into an edit field
HRESULT CTollgateCredential::SetStringValue(DWORD dwFieldID, _In_ PCWSTR pwz)
{
	HRESULT hr;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_EDIT_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft ||
			CPFT_PASSWORD_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		PWSTR* ppwszStored = &_rgFieldStrings[dwFieldID];
		CoTaskMemFree(*ppwszStored);
		hr = SHStrDupW(pwz, ppwszStored);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Returns whether a checkbox is checked or not as well as its label.
HRESULT CTollgateCredential::GetCheckboxValue(DWORD dwFieldID, _Out_ BOOL* pbChecked, _Outptr_result_nullonfailure_ PWSTR* ppwszLabel)
{
	HRESULT hr;
	*ppwszLabel = nullptr;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_CHECKBOX == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		*pbChecked = _fChecked;
		//hr = SHStrDupW(_rgFieldStrings[SFI_CHECKBOX], ppwszLabel);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Sets whether the specified checkbox is checked or not.
HRESULT CTollgateCredential::SetCheckboxValue(DWORD dwFieldID, BOOL bChecked)
{
	HRESULT hr;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_CHECKBOX == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		_fChecked = bChecked;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Returns the number of items to be included in the combobox (pcItems), as well as the
// currently selected item (pdwSelectedItem).
HRESULT CTollgateCredential::GetComboBoxValueCount(DWORD dwFieldID, _Out_ DWORD* pcItems, _Deref_out_range_(< , *pcItems) _Out_ DWORD* pdwSelectedItem)
{
	HRESULT hr;
	*pcItems = 0;
	*pdwSelectedItem = 0;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_COMBOBOX == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		*pcItems = ARRAYSIZE(s_rgComboBoxStrings);
		*pdwSelectedItem = 0;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Called iteratively to fill the combobox with the string (ppwszItem) at index dwItem.
HRESULT CTollgateCredential::GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, _Outptr_result_nullonfailure_ PWSTR* ppwszItem)
{
	HRESULT hr;
	*ppwszItem = nullptr;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_COMBOBOX == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		hr = SHStrDupW(s_rgComboBoxStrings[dwItem], ppwszItem);
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Called when the user changes the selected item in the combobox.
HRESULT CTollgateCredential::SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem)
{
	HRESULT hr;

	// Validate parameters.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_COMBOBOX == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		_dwComboIndex = dwSelectedItem;
		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Called when the user clicks a command link.
HRESULT CTollgateCredential::CommandLinkClicked(DWORD dwFieldID)
{
	HRESULT hr = S_OK;

	// Validate parameter.
	if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) &&
		(CPFT_COMMAND_LINK == _rgCredProvFieldDescriptors[dwFieldID].cpft))
	{
		switch (dwFieldID)
		{

			// --------------- USB 인증 버튼 ---------------
		case SFI_USB_VERIFY:

			EnableAuthStartButton(SFI_USB_VERIFY, FALSE);

			if (_pUSBAuth != nullptr) {
				_pUSBAuth->InitAuthThread(this);
			}
			
			//Log(LOG::ALERT::ALERT_INFO, LOG::AUTH_METHOD::AUTH_METHOD_USB, L"Test: GoToNextAuthStage()");

			break;


			// --------------- 패턴 정보 요청 버튼 ---------------
		case SFI_PATTERN_REQUEST:

			EnableAuthStartButton(SFI_PATTERN_REQUEST, FALSE);

			if (_pPatternAuth != nullptr) {
				_pPatternAuth->InitAuthThread(this);
			}
			
			break;


			// --------------- 안면 인식 정보 요청 버튼 ---------------
		case SFI_FACE_REQUEST:
			
			EnableAuthStartButton(SFI_FACE_REQUEST, FALSE);

			if (_pFaceAuth != nullptr) {
				_pFaceAuth->InitAuthThread(this);
			}

			break;


			// --------------- 지문 인식 정보 요청 버튼 ---------------
		case SFI_FINGERPRINT_REQUEST:

			EnableAuthStartButton(SFI_FINGERPRINT_REQUEST, FALSE);

			if (_pFingerprintAuth != nullptr) {
				_pFingerprintAuth->InitAuthThread(this);
			}

			break;


			// --------------- QR 정보 요청 버튼 ---------------
		case SFI_QR_REQUEST:

			EnableAuthStartButton(SFI_QR_REQUEST, FALSE);

			if (_pQRAuth != nullptr) {
				_pQRAuth->InitAuthThread(this);
			}
			
			break;

		default:
			hr = E_INVALIDARG;
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

// Collect the username and password into a serialized credential for the correct usage scenario
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials
// back to the system to log on.
HRESULT CTollgateCredential::GetSerialization(_Out_ CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	_Out_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
	_Outptr_result_maybenull_ PWSTR* ppwszOptionalStatusText,
	_Out_ CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	HRESULT hr = E_UNEXPECTED;
	*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
	*ppwszOptionalStatusText = nullptr;
	*pcpsiOptionalStatusIcon = CPSI_NONE;
	ZeroMemory(pcpcs, sizeof(*pcpcs));


	// OTP 인증 요소 사용의 경우 OTP 먼저 처리
	if (_bAuthFactorFlag & AUTH_FACTOR_OTP)
	{
		if (wcslen(_rgFieldStrings[SFI_OTP_INPUT]) == 0)
		{
			return hr;
		}

		RestClient* rc = new RestClient();

		// --------------- 서버로부터 패턴 정보 요청 ---------------
		if (rc->RequestOTPVerification(this->wszUserName, this->wszSystemIdentifier, _rgFieldStrings[SFI_OTP_INPUT]))
		{
			// --------------- 인증 서버로부터 검증 결과 값 비교하여 인증 성공 여부 판단 ---------------
			DWORD retCode = rc->GetRestClientExitCode();
			wchar_t wcMessageFromServer[2048] = { 0, };

			switch (retCode)
			{
				// 서버와 연결 성공
			case rc->RESULT_CONNECTION_SUCCESS:
				rc->GetRestClientMessage(wcMessageFromServer, 2048);

				// OTP 인증 성공 - OTP 값 일치
				if (!wcscmp(wcMessageFromServer, L"Verified"))
				{
					hr = S_OK;
				}
				// OTP 인증 실패 - OTP 값 불일치
				else
				{
					SetAuthMessage(SFI_OTP_MESSAGE, L"OTP 값이 일치하지 않습니다");
				}
				break;

				// 연결 실패
			case rc->RESULT_CONNECTION_TIMEOUT:
			case rc->RESULT_CONNECTION_FAILED:
				SetAuthMessage(SFI_OTP_MESSAGE, L"서버로부터 응답이 없습니다");
				break;

				// 설정 파일이 존재하지 않음
			case rc->RESULT_CONFIG_FILE_COMPROMISED:
				SetAuthMessage(SFI_OTP_MESSAGE, L"설정 파일이 손상되어 서버로 연결할 수 없습니다");
				break;

				// 정상적인 클라이언트 프로그램이 아님 / 타임 스탬프 일치하지 않음
			case rc->RESULT_UNAUTHORIZED_ACCESS:
			case rc->RESULT_TIMESTAMP_MISMATCH:
				SetAuthMessage(SFI_OTP_MESSAGE, L"서버에서 비정상적인 응답이 반환되었습니다");
				break;

			case rc->RESULT_UNKNOWN_ERROR:
				SetAuthMessage(SFI_OTP_MESSAGE, L"알 수 없는 오류가 발생하였습니다");
				break;
			}
		}

		delete rc;

		// 인증 결과 실패하면 OTP, 패스워드 입력 창 비움
		if (!SUCCEEDED(hr))
		{
			SetAuthMessage(SFI_OTP_INPUT, L"");
			SetAuthMessage(SFI_PASSWORD_INPUT, L"");
			return hr;
		}
	}
	
	hr = E_UNEXPECTED;

	// For local user, the domain and user name can be split from _pszQualifiedUserName (domain\username).
	// CredPackAuthenticationBuffer() cannot be used because it won't work with unlock scenario.
	if (_fIsLocalUser)
	{
		PWSTR pwzProtectedPassword;
		hr = ProtectIfNecessaryAndCopyPassword(_rgFieldStrings[SFI_PASSWORD_INPUT], _cpus, &pwzProtectedPassword);
		if (SUCCEEDED(hr))
		{
			PWSTR pszDomain;
			PWSTR pszUsername;
			hr = SplitDomainAndUsername(_pszQualifiedUserName, &pszDomain, &pszUsername);
			if (SUCCEEDED(hr))
			{
				KERB_INTERACTIVE_UNLOCK_LOGON kiul;
				hr = KerbInteractiveUnlockLogonInit(pszDomain, pszUsername, pwzProtectedPassword, _cpus, &kiul);
				if (SUCCEEDED(hr))
				{
					// We use KERB_INTERACTIVE_UNLOCK_LOGON in both unlock and logon scenarios.  It contains a
					// KERB_INTERACTIVE_LOGON to hold the creds plus a LUID that is filled in for us by Winlogon
					// as necessary.
					hr = KerbInteractiveUnlockLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);
					if (SUCCEEDED(hr))
					{
						ULONG ulAuthPackage;
						hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
						if (SUCCEEDED(hr))
						{
							pcpcs->ulAuthenticationPackage = ulAuthPackage;
							pcpcs->clsidCredentialProvider = CLSID_CSample;
							// At this point the credential has created the serialized credential used for logon
							// By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
							// that we have all the information we need and it should attempt to submit the
							// serialized credential.
							*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
						}
					}
				}
				CoTaskMemFree(pszDomain);
				CoTaskMemFree(pszUsername);
			}
			CoTaskMemFree(pwzProtectedPassword);
		}
	}
	else
	{
		DWORD dwAuthFlags = CRED_PACK_PROTECTED_CREDENTIALS | CRED_PACK_ID_PROVIDER_CREDENTIALS;

		// First get the size of the authentication buffer to allocate
		if (!CredPackAuthenticationBuffer(dwAuthFlags, _pszQualifiedUserName, const_cast<PWSTR>(_rgFieldStrings[SFI_PASSWORD_INPUT]), nullptr, &pcpcs->cbSerialization) &&
			(GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			pcpcs->rgbSerialization = static_cast<byte*>(CoTaskMemAlloc(pcpcs->cbSerialization));
			if (pcpcs->rgbSerialization != nullptr)
			{
				hr = S_OK;

				// Retrieve the authentication buffer
				if (CredPackAuthenticationBuffer(dwAuthFlags, _pszQualifiedUserName, const_cast<PWSTR>(_rgFieldStrings[SFI_PASSWORD_INPUT]), pcpcs->rgbSerialization, &pcpcs->cbSerialization))
				{
					ULONG ulAuthPackage;
					hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
					if (SUCCEEDED(hr))
					{
						pcpcs->ulAuthenticationPackage = ulAuthPackage;
						pcpcs->clsidCredentialProvider = CLSID_CSample;

						// At this point the credential has created the serialized credential used for logon
						// By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
						// that we have all the information we need and it should attempt to submit the
						// serialized credential.
						*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
					}
				}
				else
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
				}

				if (FAILED(hr))
				{
					CoTaskMemFree(pcpcs->rgbSerialization);
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}
	return hr;
}

struct REPORT_RESULT_STATUS_INFO
{
	NTSTATUS ntsStatus;
	NTSTATUS ntsSubstatus;
	PWSTR     pwzMessage;
	CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[] =
{
	{ STATUS_LOGON_FAILURE, STATUS_SUCCESS, L"비밀번호가 올바르지 않습니다", CPSI_ERROR, },
	{ STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED, L"비활성화 된 계정입니다", CPSI_WARNING },
};

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT CTollgateCredential::ReportResult(NTSTATUS ntsStatus,
	NTSTATUS ntsSubstatus,
	_Outptr_result_maybenull_ PWSTR* ppwszOptionalStatusText,
	_Out_ CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	*ppwszOptionalStatusText = nullptr;
	*pcpsiOptionalStatusIcon = CPSI_NONE;

	DWORD dwStatusInfo = (DWORD)-1;

	// Look for a match on status and substatus.
	for (DWORD i = 0; i < ARRAYSIZE(s_rgLogonStatusInfo); i++)
	{
		if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
		{
			dwStatusInfo = i;
			break;
		}
	}

	if ((DWORD)-1 != dwStatusInfo)
	{
		if (SUCCEEDED(SHStrDupW(s_rgLogonStatusInfo[dwStatusInfo].pwzMessage, ppwszOptionalStatusText)))
		{
			*pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
		}
	}

	// If we failed the logon, try to erase the password field.
	if (FAILED(HRESULT_FROM_NT(ntsStatus)))
	{
		if (_pCredProvCredentialEvents)
		{
			_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD_INPUT, L"");
			_pCredProvCredentialEvents->SetFieldString(this, SFI_OTP_INPUT, L"");
		}
	}

	// Since nullptr is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
	// this function can't fail.
	return S_OK;
}

// Gets the SID of the user corresponding to the credential.
HRESULT CTollgateCredential::GetUserSid(_Outptr_result_nullonfailure_ PWSTR* ppszSid)
{
	*ppszSid = nullptr;
	HRESULT hr = E_UNEXPECTED;
	if (_pszUserSid != nullptr)
	{
		hr = SHStrDupW(_pszUserSid, ppszSid);
	}
	// Return S_FALSE with a null SID in ppszSid for the
	// credential to be associated with an empty user tile.

	return hr;
}

// GetFieldOptions to enable the password reveal button and touch keyboard auto-invoke in the password field.
HRESULT CTollgateCredential::GetFieldOptions(DWORD dwFieldID,
	_Out_ CREDENTIAL_PROVIDER_CREDENTIAL_FIELD_OPTIONS* pcpcfo)
{
	*pcpcfo = CPCFO_NONE;

	if (dwFieldID == SFI_PASSWORD_INPUT)
	{
		*pcpcfo = CPCFO_ENABLE_PASSWORD_REVEAL;
	}
	else if (dwFieldID == SFI_TILEIMAGE)
	{
		*pcpcfo = CPCFO_ENABLE_TOUCH_KEYBOARD_AUTO_INVOKE;
	}

	return S_OK;
}


void CTollgateCredential::EnableAuthStartButton(DWORD dwFieldID, BOOL bEnable)
{
	_pCredProvCredentialEvents->BeginFieldUpdates();

	// Enable 속성이 True일 경우 버튼 활성화
	if (bEnable)
	{
		_pCredProvCredentialEvents->SetFieldInteractiveState(nullptr, dwFieldID, CPFIS_NONE);
	}
	// Enable 속성이 False일 경우 버튼 비활성화
	else
	{
		_pCredProvCredentialEvents->SetFieldInteractiveState(nullptr, dwFieldID, CPFIS_DISABLED);
	}

	_pCredProvCredentialEvents->EndFieldUpdates();
}


void CTollgateCredential::SetAuthMessage(DWORD dwFieldID, LPCWSTR strMessage)
{
	// 필드의 문자열을 세팅함
	_pCredProvCredentialEvents->BeginFieldUpdates();
	_pCredProvCredentialEvents->SetFieldString(nullptr, dwFieldID, strMessage);
	_pCredProvCredentialEvents->EndFieldUpdates();
}


// 인증이 승인되었을 경우 호출됨
void CTollgateCredential::GoToNextAuthStage()
{
	wchar_t wszStageMessage[256] = { 0, };

	switch (_eCurrentAuthStage)
	{
		// USB -> Pattern
	case AUTH_FACTOR_USB:

		_eCurrentAuthStage = AUTH_FACTOR_PATTERN;

		// 사용자가 Pattern 인증 사용 활성화
		if (_bAuthFactorFlag & AUTH_FACTOR_PATTERN)
		{
			_nAuthFactorProcessCount++;
			StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
			SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);
			SetCurrentAuthStage(AUTH_FACTOR_PATTERN);
		}
		// 사용자가 Pattern 인증 사용 비활성화
		else
		{
			GoToNextAuthStage();
		}
		break;

		// Pattern -> QR
	case AUTH_FACTOR_PATTERN:

		_eCurrentAuthStage = AUTH_FACTOR_QR;

		// 사용자가 Fingerprint 인증 사용 활성화
		if (_bAuthFactorFlag & AUTH_FACTOR_QR)
		{
			_nAuthFactorProcessCount++;
			StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
			SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);
			SetCurrentAuthStage(AUTH_FACTOR_QR);
		}
		// 사용자가 Fingerprint 인증 사용 비활성화
		else
		{
			GoToNextAuthStage();
		}
		break;

		// QR -> Fingerprint
	case AUTH_FACTOR_QR:

		_eCurrentAuthStage = AUTH_FACTOR_FINGERPRINT;

		// 사용자가 Fingerprint 인증 사용 활성화
		if (_bAuthFactorFlag & AUTH_FACTOR_FINGERPRINT)
		{
			_nAuthFactorProcessCount++;
			StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
			SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);
			SetCurrentAuthStage(AUTH_FACTOR_FINGERPRINT);
		}
		// 사용자가 Fingerprint 인증 사용 비활성화
		else
		{
			GoToNextAuthStage();
		}
		break;

		// Fingerprint -> Face
	case AUTH_FACTOR_FINGERPRINT:

		_eCurrentAuthStage = AUTH_FACTOR_FACE;

		// 사용자가 Face 인증 사용 활성화
		if (_bAuthFactorFlag & AUTH_FACTOR_FACE)
		{
			_nAuthFactorProcessCount++;
			StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
			SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);
			SetCurrentAuthStage(AUTH_FACTOR_FACE);
		}
		// 사용자가 Face 인증 사용 비활성화
		else
		{
			GoToNextAuthStage();
		}
		break;

		// Face -> OTP or Password
	case AUTH_FACTOR_FACE:

		_nAuthFactorProcessCount++;
		StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
		SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);

		// 사용자가 OTP 인증 사용 활성화 - 비밀번호와 OTP 둘 다 사용
		if (_bAuthFactorFlag & AUTH_FACTOR_OTP)
		{
			_eCurrentAuthStage = AUTH_FACTOR_OTP;
			SetCurrentAuthStage(AUTH_FACTOR_OTP | AUTH_FACTOR_PASSWORD);
		}
		// 사용자가 OTP 인증 사용 비활성화 - 비밀번호만 사용
		else
		{
			_eCurrentAuthStage = AUTH_FACTOR_PASSWORD;
			SetCurrentAuthStage(AUTH_FACTOR_PASSWORD);
		}
		break;
	}
}


// Auth Factor 플래그 변수를 이용하여 첫 번째 스테이지 세팅
void CTollgateCredential::InitializeAuthStage()
{
	if (_bAuthFactorFlag == AUTH_FACTOR_INVALID)
	{
		return;
	}

	BOOL bUseUSBFactor = _bAuthFactorFlag & AUTH_FACTOR_USB;
	BOOL bUsePatternFactor = _bAuthFactorFlag & AUTH_FACTOR_PATTERN;
	BOOL bUseQRFactor = _bAuthFactorFlag & AUTH_FACTOR_QR;
	BOOL bUseFingerprintFactor = _bAuthFactorFlag & AUTH_FACTOR_FINGERPRINT;
	BOOL bUseFaceFactor = _bAuthFactorFlag & AUTH_FACTOR_FACE;
	BOOL bUseOTPFactor = _bAuthFactorFlag & AUTH_FACTOR_OTP;
	wchar_t wszStageMessage[256] = { 0, };

	/*
	 *  사용하는 스테이지 개수 카운트
	 */
	if (bUseUSBFactor)
	{
		_nAuthFactorCount++;
	}
	if (bUsePatternFactor)
	{
		_nAuthFactorCount++;
	}
	if (bUseQRFactor)
	{
		_nAuthFactorCount++;
	}
	if (bUseFingerprintFactor)
	{
		_nAuthFactorCount++;
	}
	if (bUseFaceFactor)
	{
		_nAuthFactorCount++;
	}

	_nAuthFactorCount++;
	_nAuthFactorProcessCount++;
	StringCchPrintf(wszStageMessage, ARRAYSIZE(wszStageMessage), L"인증 단계 %d/%d 진행 중", _nAuthFactorProcessCount, _nAuthFactorCount);
	SetAuthMessage(SFI_STAGE_STATUS, wszStageMessage);

	/*
	 *  초기 스테이지 세팅
	 *  인증 순서: USB -> Pattern -> QR -> Fingerprint -> Face -> OTP & Password
	 */
	if (bUseUSBFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_USB;
		SetCurrentAuthStage(AUTH_FACTOR_USB);
	}
	else if (bUsePatternFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_PATTERN;
		SetCurrentAuthStage(AUTH_FACTOR_PATTERN);
	}
	else if (bUseQRFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_QR;
		SetCurrentAuthStage(AUTH_FACTOR_QR);
	}
	else if (bUseFingerprintFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_FINGERPRINT;
		SetCurrentAuthStage(AUTH_FACTOR_FINGERPRINT);
	}
	else if (bUseFaceFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_FACE;
		SetCurrentAuthStage(AUTH_FACTOR_FACE);
	}
	else if (bUseOTPFactor)
	{
		_eCurrentAuthStage = AUTH_FACTOR_OTP;
		SetCurrentAuthStage(AUTH_FACTOR_OTP | AUTH_FACTOR_PASSWORD);
	}
	else
	{
		_eCurrentAuthStage = AUTH_FACTOR_PASSWORD;
		SetCurrentAuthStage(AUTH_FACTOR_PASSWORD);
	}

	return;
}


void CTollgateCredential::SetCurrentAuthStage(BYTE bFlag)
{
	_pCredProvCredentialEvents->BeginFieldUpdates();

	// --------------- 전체 필드 숨김 ---------------
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_USB_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_USB_VERIFY, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PATTERN_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PATTERN_REQUEST, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_QR_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_QR_REQUEST, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FINGERPRINT_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FINGERPRINT_REQUEST, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FACE_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FACE_REQUEST, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_OTP_MESSAGE, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PASSWORD_INPUT, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PASSWORD_VERIFY, CPFS_HIDDEN);
	_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_OTP_INPUT, CPFS_HIDDEN);


	// ---------- 플래그 값 검사 후, 플래그에 해당하는 인증 요소 활성화 ----------
	if (bFlag & AUTH_FACTOR_USB)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_USB_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_USB_VERIFY, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_PATTERN)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PATTERN_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PATTERN_REQUEST, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_QR)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_QR_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_QR_REQUEST, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_FINGERPRINT)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FINGERPRINT_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FINGERPRINT_REQUEST, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_FACE)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FACE_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_FACE_REQUEST, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_OTP)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_OTP_MESSAGE, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_OTP_INPUT, CPFS_DISPLAY_IN_BOTH);
	}

	if (bFlag & AUTH_FACTOR_PASSWORD)
	{
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PASSWORD_INPUT, CPFS_DISPLAY_IN_BOTH);
		_pCredProvCredentialEvents->SetFieldState(nullptr, SFI_PASSWORD_VERIFY, CPFS_DISPLAY_IN_BOTH);
	}

	_pCredProvCredentialEvents->EndFieldUpdates();
}


