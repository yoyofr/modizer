// nsf_setup.cpp : DLL �̏��������[�`���ł��B
//

#include "stdafx.h"
#include "nsf_setup.h"

#include "../xgm/version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	����!
//
//		���� DLL �� MFC DLL �ɑ΂��ē��I�Ƀ����N�����ꍇ�A
//		MFC ���ŌĂяo����邱�� DLL ����G�N�X�|�[�g���ꂽ
//		�ǂ̊֐����֐��̍ŏ��ɒǉ������ AFX_MANAGE_STATE 
//		�}�N�����܂�ł��Ȃ���΂Ȃ�܂���B
//
//		��:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// �ʏ�֐��̖{�̂͂��̈ʒu�ɂ���܂�
//		}
//
//		���̃}�N�����e�֐��Ɋ܂܂�Ă��邱�ƁAMFC ����
//		�ǂ̌Ăяo�����D�悷�邱�Ƃ͔��ɏd�v�ł��B
//		����͊֐����̍ŏ��̃X�e�[�g�����g�łȂ���΂�
//		��Ȃ����Ƃ��Ӗ����܂��A�R���X�g���N�^�� MFC 
//		DLL ���ւ̌Ăяo�����s���\��������̂ŁA�I�u
//		�W�F�N�g�ϐ��̐錾�����O�łȂ���΂Ȃ�܂���B
//
//		�ڍׂɂ��Ă� MFC �e�N�j�J�� �m�[�g 33 �����
//		58 ���Q�Ƃ��Ă��������B
//

// Cnsf_setupApp

BEGIN_MESSAGE_MAP(Cnsf_setupApp, CWinApp)
END_MESSAGE_MAP()


// Cnsf_setupApp �R���X�g���N�V����

Cnsf_setupApp::Cnsf_setupApp()
{
	// TODO: ���̈ʒu�ɍ\�z�p�R�[�h��ǉ����Ă��������B
	// ������ InitInstance ���̏d�v�ȏ��������������ׂċL�q���Ă��������B
}


// �B��� Cnsf_setupApp �I�u�W�F�N�g�ł��B

Cnsf_setupApp theApp;


// Cnsf_setupApp ������
static NSFDialogManager *pDlg = NULL;

static HANDLE hDbgfile;

BOOL Cnsf_setupApp::InitInstance()
{
	CWinApp::InitInstance();
#if defined(_MSC_VER)
#ifdef _DEBUG
   hDbgfile = CreateFile("C:\\in_nsf_ui.log", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) ;
  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ) ;
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE ) ;
  _CrtSetReportFile( _CRT_WARN, (HANDLE)hDbgfile ) ;
#endif
#endif
	return TRUE;
}

int Cnsf_setupApp::ExitInstance()
{
  return CWinApp::ExitInstance();
}

extern "C" __declspec( dllexport ) NSFplug_UI *CreateNSFplug_UI(NSFplug_Model *cf, int mode)
{
  return new NSFDialogManager(cf,mode);
}

extern "C" __declspec( dllexport ) void DeleteNSFplug_UI(NSFplug_UI *p)
{
  delete p;
}

extern "C" __declspec( dllexport ) const char* VersionNSFplug_UI()
{
  return NSFPLAY_VERSION;
}
