// win32.cpp : �A�v���P�[�V�����̃N���X������`���܂��B
//

#include "stdafx.h"
#include "win32.h"
#include "vcmDlg.h"
#include "vcmTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace vcm;

// Cwin32App

BEGIN_MESSAGE_MAP(Cwin32App, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// Cwin32App �R���X�g���N�V����

Cwin32App::Cwin32App() 
: m_cg( "���ʂ̐ݒ�", "�e�X�g" ), 
  m_sub1( "�ǂ��ݒ�", "ttt"),
  m_sub2( "�����ݒ�", "t"),
  m_sub3( "�\�̐ݒ�", "")
{

  // �R���t�B�O�����
  m_config.CreateValue("test", 8 );
  m_config.CreateValue("test2", true );
  m_config.CreateValue("test3", "������" );
  m_config.CreateValue("test4", true );
  m_config.CreateValue("test5", 16 );
  m_config.CreateValue("test6", 32 );
  m_config.CreateValue("test7", (16*3600+17*60+18)*1000 );

  // �J�e�S���[�����
  m_cg.Insert( "test5", new VT_SPIN   ( "���l�{�b�N�X(&I)", "�e�X�g�p���l�{�b�N�X", 0, 16 ) );
  m_cg.Insert( "test",  new VT_SPIN   ( "���l�{�b�N�X2(&J)", "�e�X�g�p���l�{�b�N�X", 0, 256 ) );
  m_sub1.Insert( "test2", new VT_CHECK( "�`�F�b�N�{�b�N�X(&B)", "�e�X�g�p�`�F�b�N�{�b�N�X" ) );
  m_sub1.Insert( "test3", new VT_TEXT ( "������{�b�N�X(&S)",  "�e�X�g�p������{�b�N�X", 16 ) );
  m_cg.Insert( "test4", new VT_CHECK  ( "�`�F�b�N�{�b�N�X����2(&C)", "�e�X�g�p�`�F�b�N�{�b�N�X" ) );
  m_cg.Insert( "test5", new VT_SLIDER ( "�X���C�_�[(&L)", "�e�X�g�p�X���C�_�[", 0, 32, "�ŏ�", "�ő�", 4, 4 ) );


  std::vector<std::string> items;
  items.push_back("����1");
  items.push_back("����2");
  items.push_back("��������3");
  items.push_back("������");

  std::map<std::string, int> map;
  map["����1"]=0;
  map["����2"]=1;
  map["��������3"]=2;
  map["������"]=16;

  m_cg.Insert( "test3", new VT_COMBO ( "�R���{�{�b�N�X(&X)", "����ڂ���", items ) );
  m_sub2.Insert( "test",  new VT_COMBO_INT( "�����l�������̃R���{","�R���{�Ȃ�", items, map ) );

  class TestConv : public ValueConv
  {
    virtual bool GetExportValue( ValueCtrl *vt, Configuration &cfg, const std::string &id, const Value &value, Value &result )
    {
      if( (int)cfg["test5"] < 16 )
        return false;
      result = value;
      return true;
    }
  };

  m_cg.Insert( "test6", new VT_SLIDER ( "�X���C�_�[����2(&M)", "�e�X�g�p�X���C�_�[", 0, 32, "MIN ", "MAX ", 4, 8 ), new TestConv() ); 
  m_sub2.Insert( "test7", new VT_TEXT ( "���ԃ{�b�N�X(&T)", "�����\��", 8 ), new VC_TIME() );
  m_cg.Insert( "test",  new VT_TEXT ( "������{�b�N�X",  "�e�X�g�p������{�b�N�X", 16 ) );

}

// �B��� Cwin32App �I�u�W�F�N�g�ł��B

Cwin32App theApp;


// Cwin32App ������

BOOL Cwin32App::InitInstance()
{
	// �A�v���P�[�V�����@�}�j�t�F�X�g���@visual �X�^�C����L���ɂ��邽�߂ɁA
	// ComCtl32.dll �o�[�W���� 6�@�ȍ~�̎g�p���w�肷��ꍇ�́A
	// Windows XP �Ɂ@InitCommonControls() ���K�v�ł��B�����Ȃ���΁A�E�B���h�E�쐬�͂��ׂĎ��s���܂��B
	InitCommonControls();

	CWinApp::InitInstance();

	CvcmTree dlg;
	m_pMainWnd = &dlg;

  m_cg.AddSubGroup(&m_sub1);
  m_sub2.AddSubGroup(&m_sub3);
  dlg.AttachConfig(m_config);
  dlg.AttachGroup(m_cg);
  dlg.AttachGroup(m_sub2);
	
  INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �_�C�A���O�� <OK> �ŏ����ꂽ���̃R�[�h��
		//       �L�q���Ă��������B
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �_�C�A���O�� <�L�����Z��> �ŏ����ꂽ���̃R�[�h��
		//       �L�q���Ă��������B
	}

	// �_�C�A���O�͕����܂����B�A�v���P�[�V�����̃��b�Z�[�W �|���v���J�n���Ȃ���
	// �A�v���P�[�V�������I�����邽�߂� FALSE ��Ԃ��Ă��������B
	return FALSE;
}
