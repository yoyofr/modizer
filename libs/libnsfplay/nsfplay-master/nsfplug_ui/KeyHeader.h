#pragma once


// KeyHeader �_�C�A���O

class KeyHeader : public CDialog
{
	DECLARE_DYNAMIC(KeyHeader)

public:
	KeyHeader(CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~KeyHeader();

  // �L�[�{�[�h�w�b�_�̌��G
  CBitmap m_hedBitmap;
  CDC m_hedDC;

  // ����ʗ̈�
  CBitmap m_memBitmap;
  CDC m_memDC;

  // �y��
  CPen softgray_pen;

  int m_nNoiseStatus;
  int m_nDPCMStatus;

  virtual void Reset();

  inline int MinWidth(){ return 336+1; }
  inline int MaxWidth(){ return 336*2+1; }
  inline int MinHeight(){ return 24; }
  inline int MaxHeight(){ return 24; }

// �_�C�A���O �f�[�^
	enum { IDD = IDD_KEYHEADER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg void OnPaint();
};
