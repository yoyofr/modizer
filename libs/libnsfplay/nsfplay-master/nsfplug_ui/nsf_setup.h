// nsf_setup.h : nsf_setup.DLL �̃��C�� �w�b�_�[ �t�@�C��

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ���C�� �V���{��
#include "NSFDialogs.h"

// Cnsf_setupApp
// ���̃N���X�̎����Ɋւ��Ă� nsf_setup.cpp ���Q�Ƃ��Ă��������B
//

class Cnsf_setupApp : public CWinApp
{
protected:
public:
	Cnsf_setupApp();

// �I�[�o�[���C�h
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
  virtual int ExitInstance();
};

