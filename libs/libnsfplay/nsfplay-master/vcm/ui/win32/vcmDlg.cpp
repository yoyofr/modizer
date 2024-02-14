// vcmDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "win32.h"
#include "vcmDlg.h"
#include "vcmSpin.h"
#include "vcmSlider.h"
#include "vcmCheck.h"
#include "vcmCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace vcm;

// CvcmDlg �_�C�A���O

CvcmDlg::CvcmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CvcmDlg::IDD, pParent), m_pConfig(NULL), m_pGroup(NULL), m_dlgSize(64,64)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CvcmDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CvcmDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
  ON_WM_DESTROY()
  ON_WM_SIZE()
  ON_WM_VSCROLL()
  ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CvcmDlg ���b�Z�[�W �n���h��

BOOL CvcmDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���̃_�C�A���O�̃A�C�R����ݒ肵�܂��B�A�v���P�[�V�����̃��C�� �E�B���h�E���_�C�A���O�łȂ��ꍇ�A
	//  Framework �́A���̐ݒ�������I�ɍs���܂��B
	SetIcon(m_hIcon, TRUE);			// �傫���A�C�R���̐ݒ�
	SetIcon(m_hIcon, FALSE);		// �������A�C�R���̐ݒ�

  // �R���t�B�O�f�[�^�̎��o��
  m_ctrlPos.SetPoint(0,0);
  m_dlgSize.SetSize(0,0);

  ConfigGroup::GroupList::iterator it;

  for( it=m_pGroup->members.begin(); it!=m_pGroup->members.end(); it++)
  {
    CvcmCtrl *pCtrl;
    CRect rect;

    switch(it->second->ctrlType)
    {
    case ValueCtrl::CT_CHECK:
      pCtrl = new CvcmCheck( it->first, (VT_CHECK *)it->second, this );
      pCtrl->AttachConfig( *m_pConfig );
      ((CvcmCheck *)pCtrl)->Create( CvcmCheck::IDD, this );
      break;
    
    case ValueCtrl::CT_SPIN:
      pCtrl = new CvcmSpin( it->first, (VT_SPIN *)it->second, this );
      pCtrl->AttachConfig( *m_pConfig );
      ((CvcmSpin *)pCtrl)->Create( CvcmSpin::IDD, this );
      break;
    
    case ValueCtrl::CT_TEXT:
      pCtrl = new CvcmText( it->first, (VT_TEXT *)it->second, this );
      pCtrl->AttachConfig( *m_pConfig );
      ((CvcmText *)pCtrl)->Create( CvcmText::IDD, this );
      break;
    
    case ValueCtrl::CT_SLIDER:
      pCtrl = new CvcmSlider( it->first, (VT_SLIDER *)it->second, this );
      pCtrl->AttachConfig( *m_pConfig );
      ((CvcmSlider *)pCtrl)->Create( CvcmSlider::IDD, this );
      break;

    case ValueCtrl::CT_COMBO:
      pCtrl = new CvcmCombo( it->first, (VT_COMBO *)it->second, this );
      pCtrl->AttachConfig( *m_pConfig );
      ((CvcmCombo *)pCtrl)->Create( CvcmCombo::IDD, this );
      break;

    default:
      pCtrl = NULL;
      break;
    }

    if(pCtrl)
    {
      m_ctrl2id[pCtrl] = it->first;

      pCtrl->GetWindowRect(rect);      
      pCtrl->SetWindowPos( NULL, m_ctrlPos.x, m_ctrlPos.y, rect.Width(), rect.Height()*3/2, SWP_NOZORDER);
      m_ctrlPos.y += rect.Height() * 3 / 2;

      if( m_dlgSize.cx < rect.Width() )
        m_dlgSize.cx = rect.Width()+8;
      if( m_dlgSize.cy < m_ctrlPos.y )
        m_dlgSize.cy = m_ctrlPos.y ;
    }

    m_nPosV = m_nPosH = 0;

    SetWindowPos ( NULL, 0, 0, m_dlgSize.cx, m_dlgSize.cy, SWP_NOMOVE|SWP_NOZORDER );
  }

  m_block_update = false;
  UpdateConfig(false);
	
	return TRUE;  // �t�H�[�J�X���R���g���[���ɐݒ肵���ꍇ�������ATRUE ��Ԃ��܂��B
}

void CvcmDlg::AttachConfig( Configuration &config )
{
  m_pConfig = &config;
}

void CvcmDlg::AttachGroup( ConfigGroup &group )
{
  m_pGroup = &group;
}

void CvcmDlg::UpdateConfig(bool b)
{
  if(!m_block_update)
  {
    std::map<CvcmCtrl*,std::string>::iterator it;
    if(b) // �_�C�A���O���R���t�B�O
    {
      for(it=m_ctrl2id.begin();it!=m_ctrl2id.end();it++)
        it->first->WriteWork();
    } 
    else  // �R���t�B�O���_�C�A���O
    {
      for(it=m_ctrl2id.begin();it!=m_ctrl2id.end();it++)
        it->first->ReadWork();
    }
  }
}

// �_�C�A���O�ɍŏ����{�^����ǉ�����ꍇ�A�A�C�R����`�悷�邽�߂�
//  ���̃R�[�h���K�v�ł��B�h�L�������g/�r���[ ���f�����g�� MFC �A�v���P�[�V�����̏ꍇ�A
//  ����́AFramework �ɂ���Ď����I�ɐݒ肳��܂��B

void CvcmDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �`��̃f�o�C�X �R���e�L�X�g

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �N���C�A���g�̎l�p�`�̈���̒���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �A�C�R���̕`��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���[�U�[���ŏ��������E�B���h�E���h���b�O���Ă���Ƃ��ɕ\������J�[�\�����擾���邽�߂ɁA
//  �V�X�e�������̊֐����Ăяo���܂��B
HCURSOR CvcmDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CvcmDlg::OnDestroy()
{
  CDialog::OnDestroy();

  m_block_update = true;

  std::map< CvcmCtrl *, std::string >::iterator it;
  for(it=m_ctrl2id.begin();it!=m_ctrl2id.end();it++)
  {
    it->first->DestroyWindow();
    delete it->first;
  }
  m_ctrl2id.clear();

}

void CvcmDlg::UpdateStatusMessage( const std::string &str )
{
}

void CvcmDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);

  CRect rect(0,0,0,0);

  if( m_dlgSize.cy > cy )
  {
    ShowScrollBar(SB_VERT);
    SCROLLINFO sif;
    sif.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    sif.nPos = 0;
    sif.nMin = 0;
    sif.nMax = m_dlgSize.cy - cy;
    sif.nPage = sif.nMax / 4 ;
    SetScrollInfo( SB_VERT, &sif );
    ScrollWindow( 0, m_nPosV );
    m_nPosV = 0;
  }
  else
    ShowScrollBar(SB_VERT,FALSE);

  if( m_dlgSize.cx > cx )
  {
    ShowScrollBar(SB_HORZ);
    SCROLLINFO sif;
    sif.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    sif.nPos = 0;
    sif.nMin = 0;
    sif.nMax = m_dlgSize.cx - cx ;
    sif.nPage = sif.nMax / 4 ;
    SetScrollInfo( SB_HORZ, &sif );
    ScrollWindow( m_nPosH, 0 );
    m_nPosH = 0;
  }
  else
    ShowScrollBar(SB_HORZ,FALSE);
}


void CvcmDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  SCROLLINFO sif;
  GetScrollInfo(SB_VERT, &sif);

  switch( nSBCode )
  {
  case SB_THUMBTRACK:
  case SB_THUMBPOSITION:
    sif.nPos = nPos;
    break;

  case SB_PAGEDOWN:
    sif.nPos += sif.nPage;
    break;

  case SB_LINEDOWN:
    sif.nPos ++;
    break;

  case SB_PAGEUP:
    sif.nPos -= sif.nPage;
    break;

  case SB_LINEUP:
    sif.nPos --;
    break;

  default:
    break;
  }

  SetScrollInfo( SB_VERT, &sif );
  ScrollWindow( 0, m_nPosV - GetScrollPos(SB_VERT) );
  m_nPosV = GetScrollPos(SB_VERT);
  UpdateWindow();

  CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CvcmDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  SCROLLINFO sif;
  GetScrollInfo(SB_HORZ, &sif);

  switch( nSBCode )
  {
  case SB_THUMBTRACK:
  case SB_THUMBPOSITION:
    sif.nPos = nPos;
    break;

  case SB_PAGERIGHT:
    sif.nPos += sif.nPage;
    break;

  case SB_LINERIGHT:
    sif.nPos ++;
    break;

  case SB_PAGELEFT:
    sif.nPos -= sif.nPage;
    break;

  case SB_LINELEFT:
    sif.nPos --;
    break;

  default:
    break;
  }

  SetScrollInfo( SB_HORZ, &sif );
  ScrollWindow( m_nPosH - GetScrollPos(SB_HORZ), 0 );
  m_nPosH = GetScrollPos(SB_HORZ);
  UpdateWindow();

  CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
