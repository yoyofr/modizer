// win32.h : PROJECT_NAME �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C���ł��B
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ���C�� �V���{��
#include "../../vcm.h"

// Cwin32App:
// ���̃N���X�̎����ɂ��ẮAwin32.cpp ���Q�Ƃ��Ă��������B
//

class Cwin32App : public CWinApp
{
public:
	Cwin32App();

// �I�[�o�[���C�h
	public:
	virtual BOOL InitInstance();

// ����
public:
  vcm::Configuration m_config;
  vcm::ConfigGroup m_cg, m_sub1, m_sub2, m_sub3;

	DECLARE_MESSAGE_MAP()
};

extern Cwin32App theApp;
