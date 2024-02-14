// NSFInfoDialog.cpp : �C���v�������e�[�V���� �t�@�C��
//
#include "stdafx.h"
#include "NSFDialogs.h"
#include "NSFInfoDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// NSFInfoDialog �_�C�A���O


NSFInfoDialog::NSFInfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(NSFInfoDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(NSFInfoDialog)
	m_song = 0;
	m_artist = _T("");
	m_copyright = _T("");
	m_title = _T("");
	m_info = _T("");
	m_fds = _T("");
	m_fme7 = _T("");
	m_mmc5 = _T("");
	m_n106 = _T("");
	m_vrc6 = _T("");
	m_vrc7 = _T("");
    m_pal = _T("");
	//}}AFX_DATA_INIT
}

void NSFInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NSFInfoDialog)
	DDX_Control(pDX, IDC_SONGSLIDER, m_songslider);
	DDX_Text(pDX, IDC_SONG, m_song);
	DDX_Text(pDX, IDC_FDS, m_fds);
	DDX_Text(pDX, IDC_FME7, m_fme7);
	DDX_Text(pDX, IDC_MMC5, m_mmc5);
	DDX_Text(pDX, IDC_N106, m_n106);
	DDX_Text(pDX, IDC_VRC6, m_vrc6);
	DDX_Text(pDX, IDC_VRC7, m_vrc7);
	DDX_Text(pDX, IDC_PAL, m_pal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(NSFInfoDialog, CDialog)
	//{{AFX_MSG_MAP(NSFInfoDialog)
	ON_COMMAND(ID_MEMDLG, OnMemdlg)
	//ON_COMMAND(ID_EASY, OnEasy)
	ON_COMMAND(ID_CONFIG, OnConfigBox)
	ON_COMMAND(ID_MIXERBOX, OnMixerbox)
	ON_COMMAND(ID_PANBOX, OnPanBox)
	ON_COMMAND(ID_FILE_ABOUT, OnAbout)
	ON_COMMAND(ID_SAVE, OnSave)
	ON_COMMAND(ID_LOAD, OnLoad)
	ON_COMMAND(ID_GENPLS, OnGenpls)
	ON_COMMAND(ID_MASKBOX, OnMaskbox)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREV, OnPrev)
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
  ON_COMMAND(ID_DELALL, OnDelall)
  ON_COMMAND(ID_DELONE, OnDelone)
//  ON_COMMAND(ID_USETAG, OnUsetag)
ON_COMMAND(ID_READTAG, OnReadtag)
ON_COMMAND(ID_WRITETAG, OnWritetag)
ON_COMMAND(ID_TRKINFO, OnTrkinfo)
ON_STN_CLICKED(IDC_ARTIST, OnStnClickedArtist)
ON_WM_DROPFILES()
ON_COMMAND(ID_NEWPLS, OnNewpls)
ON_COMMAND(ID_PRESET, OnLoadpreset)
END_MESSAGE_MAP()

// sets UTF8 text on a dialog item
void SetTextW(CWnd* cw, int item, CString& s)
{
  CArray<wchar_t,int> w;
  w.SetSize(utf8_file(s.GetBuffer(),NULL,0));
  if (w.GetSize() == 0) w.SetSize(1);
  utf8_file(s.GetBuffer(),w.GetData(),w.GetSize());
  SetWindowTextW(cw->GetDlgItem(item)->GetSafeHwnd(),w.GetData());
}

/////////////////////////////////////////////////////////////////////////////
// NSFInfoDialog �̋@�
void NSFInfoDialog::SetInfo(NSF *nsf)
{
  CString ss;
  static char buf[4096], *p;
  int i=0;

  m_title = (CString) nsf->title;
  m_artist = (CString) nsf->artist;
  m_copyright = (CString) nsf->copyright;
  
  m_fds =  (CString)(nsf->use_fds?"FDS":"");
  m_mmc5 = (CString)(nsf->use_mmc5?"MMC5":"");
  m_fme7 = (CString)(nsf->use_fme7?"5B":"");
  m_vrc6 = (CString)(nsf->use_vrc6?"VRC6":"");
  m_vrc7 = (CString)(nsf->use_vrc7?"VRC7":"");
  m_n106 = (CString)(nsf->use_n106?"N163":"");

  const char* REGN_STRING[8] =
  {
      "None", "NTSC", "PAL", "Dual",
      "Dendy", "D.N", "DP.", "DPN"
  };
  m_pal = (CString)REGN_STRING[nsf->regn];

  m_info = (CString)(buf);
  m_song = (nsf->song+1);
  m_songslider.SetRange(1, nsf->songs);
  m_songslider.SetTicFreq(1);
  m_songslider.SetPos(m_song);
  m_songslider.SetPageSize(16);
  m_songslider.SetLineSize(1);

  nsf_copy = (*nsf);
  nsf_copy.body=NULL; // �w�b�_�̂݃R�s�[  
  nsf_copy.nsfe_image=NULL;
  ntag.SetNSF(&nsf_copy);

  // if NSFe text chunk is present, reformat newlines and ensure string termination
  if (nsf->text != NULL)
  {
    for (xgm::UINT32 i=0; i < nsf->text_len && nsf->text[i] != 0; ++i)
    {
        char c = nsf->text[i];
        if      (c == '\r')
        {
            continue; // ignore CR
        }
        else if (c == '\n')
        {
            m_info += "\r\n"; // expand LF to CR+LF (for windows text view)
        }
        else
        {
            m_info += c;
        }
    }
    ss.Format("\r\n--- end of NSFe text ---\r\n\r\n");
    m_info += ss;
  }

  if((int)CONFIG["WRITE_TAGINFO"]&&!ntag.IsExistSection(true)&&!ntag.IsExistSection(false))
    ntag.CreateTag(CONFIG["TITLE_FORMAT"]);

  // convert backslashes (in Shift JIS backslash is Yen)
  {
    m_info += "FILE=";
    for(const char* c = nsf->filename; *c != 0; ++c)
    {
        if (*c == '\\')
        {
            m_info += '/';
        }
        else
        {
            m_info += *c;
        }
    }    
    m_info += "\r\n";
  }

  ss.Format(
      "Start track: %d of %d\r\n"
      "NTSC Speed: %fHz (%d)\r\n"
      "PAL Speed: %fHz (%d)\r\n"
      "Dendy Speed: %fHz (%d)\r\n"
      "Banks: %02X %02X %02X %02X %02X %02X %02X %02X\r\n"
      "Load/Init/Play: %04X/%04X/%04X\r\n"
      , nsf->start, nsf->songs
      , 1000000.0 / nsf->speed_ntsc, nsf->speed_ntsc
      , 1000000.0 / nsf->speed_pal, nsf->speed_pal
      , 1000000.0 / nsf->speed_dendy, nsf->speed_dendy
      , nsf->bankswitch[0]
      , nsf->bankswitch[1]
      , nsf->bankswitch[2]
      , nsf->bankswitch[3]
      , nsf->bankswitch[4]
      , nsf->bankswitch[5]
      , nsf->bankswitch[6]
      , nsf->bankswitch[7]
      , nsf->load_address, nsf->init_address, nsf->play_address
      );
  m_info += ss;

  if (nsf->ripper[0] != 0)
  {
    ss.Format("Ripper: %s\r\n", nsf->ripper);
    m_info += ss;
  }

  for (unsigned int i=0; i < nsf->total_songs; ++i)
  {
    if (nsf->nsfe_entry[i].tlbl[0] != 0  ||
        nsf->nsfe_entry[i].time    != -1 ||
        nsf->nsfe_entry[i].fade    != -1 )
    {
      ss.Format("NSFe track %d: %s, %s, %d, %d%s\r\n",
          i,
          nsf->nsfe_entry[i].tlbl[0]!=0 ? nsf->nsfe_entry[i].tlbl : "(untitled)",
          nsf->nsfe_entry[i].taut[0]!=0 ? nsf->nsfe_entry[i].taut : nsf->artist,
          nsf->nsfe_entry[i].time,
          nsf->nsfe_entry[i].fade,
          nsf->nsfe_entry[i].psfx ? " SFX": ""
          );
      m_info += ss;
    }
  }

  if (nsf->nsfe_plst)
  {
    m_info += "NSFe plst order:";
    for (int i=0; i < nsf->nsfe_plst_size; ++i)
    {
      ss.Format(" %d", nsf->nsfe_plst[i]);
      m_info += ss;
      if (i < (nsf->nsfe_plst_size - 1))
      {
        m_info += ",";
      }
    }
    m_info += "\r\n";
  }

  if(ntag.IsExistSection())
  {
    ss.LoadString(IDS_FOUNDTAG);
    m_info += ss + "\r\n";
    if(!ntag.IsWriteEnable())
    {
      ss.LoadString(IDS_READONLYTAG);
      m_info += ss + "\r\n";
    }
  }

  local_tag = ntag.IsExistSection(true);
  CMenu *menu = GetMenu();
  if(menu)
  {
    menu->EnableMenuItem(ID_DELONE, ntag.IsWriteEnable()&&!local_tag?MF_ENABLED:MF_GRAYED);
    menu->EnableMenuItem(ID_DELALL, ntag.IsWriteEnable()&&!local_tag?MF_ENABLED:MF_GRAYED);
  }

  if(local_tag)
  {
    local_tag = true;
    ss.LoadString(IDS_LOCALTAG);
    m_info += ss + "\r\n";
  }

  GetDlgItem(IDC_SONGSLIDER)->EnableWindow(nsf->enable_multi_tracks?TRUE:FALSE);
  GetDlgItem(IDC_PREV)->EnableWindow(nsf->enable_multi_tracks?TRUE:FALSE);
  GetDlgItem(IDC_NEXT)->EnableWindow(nsf->enable_multi_tracks?TRUE:FALSE);

  UpdateData(false);
  //GetDlgItem(IDC_INFO)->SetWindowTextA(m_winfo.GetBuffer());

  // convert UTF8 to unicode
  SetTextW(this,IDC_INFO,m_info);
  SetTextW(this,IDC_ARTIST,m_artist);
  SetTextW(this,IDC_COPYRIGHT,m_copyright);
  SetTextW(this,IDC_TITLE,m_title);
}

void NSFInfoDialog::SetInfo(const char *fn)
{
  if(nsf.LoadFile(fn)==false)
  {
    GetDlgItem(IDC_INFO)->SetWindowText("Not an NSF file");
    GetDlgItem(IDC_ARTIST)->SetWindowText("");
    GetDlgItem(IDC_COPYRIGHT)->SetWindowText("");
    GetDlgItem(IDC_TITLE)->SetWindowText("");
    return;
  }
  if(CONFIG["NSFE_PLAYLIST"] && nsf.nsfe_plst)
  {
    nsf.songs = nsf.nsfe_plst_size;
  }

  SetInfo(&nsf);
}

// �v���C���X�g����
void NSFInfoDialog::GeneratePlaylist(bool clear)
{
  if (ntag.sdat == NULL)
  {
      MessageBox("No NSF loaded.","Fatal Error");
      return;
  }

  int i;
  FILE *fp;
  char path_buffer[_MAX_PATH];
  NSF local_nsf(*ntag.sdat);
  local_nsf.body = NULL;
  local_nsf.nsfe_image = NULL;
  NSF_TAG local_ntag(&local_nsf);

  _makepath( path_buffer, ntag.drv, ntag.dir, ntag.fname, "pls" ); 

  if(!parent||!parent->hWinamp)
  {
    MessageBox("Can't find Winamp.","Fatal Error");
    return;
  }

  fp = fopen_utf8(path_buffer,"r");
  if(fp)
  {
    fclose(fp);
    if(IDNO==MessageBox("Playlist file already exists. Overwrite?", path_buffer, MB_YESNO|MB_ICONWARNING))
      return;
  }

  fp = fopen_utf8(path_buffer, "w");
  if(fp==NULL)
  {
    MessageBox("Playlist Write Error!", "Error", MB_OK|MB_ICONEXCLAMATION );
    return;
  }

  fprintf(fp, "[playlist]\n");
  nsf.playlist_mode = false;
  for(i=0;i<nsf.songs;i++)
  {
    local_nsf.SetSong(i);
    local_ntag.ReadTagItem(i);
    fprintf(fp, "File%d=%s\n", i+1, 
      local_nsf.GetPlaylistString(
        (*(parent->GetConfig()))["TITLE_FORMAT"],
        (*(parent->GetConfig()))["READ_TAGINFO"].GetInt()?true:false
      )
    );
  }
  fprintf(fp, "NumberOfEntries=%d\n",i);
  fprintf(fp, "Version=2\n");
  fclose(fp);

  if(clear) 
    ::SendMessage(parent->hWinamp,WM_WA_IPC,0,IPC_DELETE);

  COPYDATASTRUCT cds;
  cds.dwData = IPC_PLAYFILE;
  cds.lpData = (void *)path_buffer;
  cds.cbData = (DWORD)strlen((char *) cds.lpData)+1;
  ::SendMessage(parent->hWinamp,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
  ::PostMessage(parent->hWinamp, WM_COMMAND, WINAMP_BUTTON2, 0) ;
  return;

}

/////////////////////////////////////////////////////////////////////////////
// NSFInfoDialog ���b�Z�[�W �n���h��

void NSFInfoDialog::OnNewpls()
{
  GeneratePlaylist(true);
}

void NSFInfoDialog::OnGenpls() 
{
  GeneratePlaylist(false);
}

void NSFInfoDialog::OnMemdlg() 
{
	parent->memory->Open();
}

void NSFInfoDialog::OnEasy() 
{
  parent->easy->Open();	
}

void NSFInfoDialog::OnConfigBox() 
{
	parent->config->Open();
}

void NSFInfoDialog::OnMixerbox() 
{
  parent->mixer->Open();
}

void NSFInfoDialog::OnPanBox() 
{
  parent->panner->Open();
}

BOOL NSFInfoDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// store initial positions
	CRect rect;
	GetClientRect(rect); m_base_size = rect.Size(); // OnSize uses ClientRect size
	GetWindowRect(rect); m_base_min  = rect.Size(); // OnGetMinMaxInfo uses WindowRect size
	for (int i=0; i<CHILDREN; ++i)
	{
		GetDlgItem(m_child[i])->GetWindowRect(m_child_rect[i]);
		ScreenToClient(m_child_rect[i]); // MoveWindow uses WindowRect relative to Client
	}

  HICON hIcon = AfxGetApp()->LoadIcon(IDI_NSF);
  SetIcon(hIcon, TRUE);
  CMenu *menu = GetMenu();
  if(menu)
  {
    menu->CheckMenuItem(ID_READTAG, (int)CONFIG["READ_TAGINFO"]?MF_CHECKED:MF_UNCHECKED);
    menu->CheckMenuItem(ID_WRITETAG, (int)CONFIG["WRITE_TAGINFO"]?MF_CHECKED:MF_UNCHECKED);
    menu->EnableMenuItem(ID_DELONE, true?MF_ENABLED:MF_GRAYED);
    menu->EnableMenuItem(ID_DELALL, true?MF_ENABLED:MF_GRAYED);
  }
	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void NSFInfoDialog::OnSave() 
{
  CFileDialog fd(FALSE,".ini","in_yansf",OFN_OVERWRITEPROMPT,"INI files (*.ini)|*.ini||",this);

  if(fd.DoModal()==IDOK)
    pm->cf->Save(fd.GetPathName().GetBuffer(1),"NSFPlug");
}

void NSFInfoDialog::OnAbout()
{
    ::MessageBox(NULL,
        "NSFPlug " NSFPLAY_VERSION "\n"
        "Brad Smith, " __DATE__ "\n"
        "http://rainwarrior.ca/\n"
        "\n"
        "Original by Brezza, Dec 19 2006\n"
        "http://pokipoki.org/dsa/\n"
        ,
        "About",
        MB_OK);
}

void NSFInfoDialog::OnLoad() 
{
  CFileDialog fd(TRUE,".ini","in_yansf",OFN_FILEMUSTEXIST,"INI files (*.ini)|*.ini||",this);

  if(fd.DoModal()==IDOK)
  {
    pm->cf->Load(fd.GetPathName().GetBuffer(1),"NSFPlug");
    pm->cf->Notify(-1);
  }

}

void NSFInfoDialog::OnMaskbox() 
{
	parent->mask->Open();	
}

void NSFInfoDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  if(pScrollBar == (CScrollBar *)&m_songslider)
  {
    if(nSBCode == TB_THUMBTRACK)
    {
      m_song = nPos;
      UpdateData(false);
    }
    else if(nSBCode == TB_ENDTRACK)
    {
      m_song = m_songslider.GetPos();
      if(UpdateData(false))
        ::SendMessage(parent->hWinamp, WM_USER+2413, (WPARAM)0xdeadbeef, (LPARAM)m_song-1);
    }
  }
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void NSFInfoDialog::OnPrev()
{
  int min, max;
  m_songslider.GetRange(min, max);
  m_song = m_song - 1;
  if((int)m_song < min) m_song = min;
	if(UpdateData(false))
    ::SendMessage(parent->hWinamp, WM_USER+2413, (WPARAM)0xdeadbeef, (LPARAM)m_song-1);
}

void NSFInfoDialog::OnNext() 
{
  int min, max;
  m_songslider.GetRange(min, max);
  m_song = m_song + 1;
  if(max < (int)m_song ) m_song = max;
  if(UpdateData(false))
    ::SendMessage(parent->hWinamp, WM_USER+2413, (WPARAM)0xdeadbeef, (LPARAM)m_song-1);
}

//
// dynamic resizing
//

const int NSFInfoDialog::m_child[CHILDREN] = {
	IDC_HEADER,
	IDC_MISC,
	IDC_INFO,
	IDC_SONGBOX,
	IDC_SONGSTATIC,
	IDC_SONG,
	IDC_SONGSLIDER,
	IDC_PREV,
	IDC_NEXT,
};

// whether they move and/or resize with the window
#define RESIZE_MX 0x01
#define RESIZE_MY 0x02
#define RESIZE_SX 0x04
#define RESIZE_SY 0x08
const int NSFInfoDialog::m_child_resize[CHILDREN] = {
	RESIZE_SX,
	RESIZE_SX | RESIZE_SY,
	RESIZE_SX | RESIZE_SY,
	RESIZE_SX | RESIZE_MY,
	RESIZE_MY,
	RESIZE_MY,
	RESIZE_SX | RESIZE_MY,
	RESIZE_MX | RESIZE_MY,
	RESIZE_MX | RESIZE_MY,
};

void NSFInfoDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CDialog::OnGetMinMaxInfo(lpMMI);
	if (lpMMI->ptMinTrackSize.x < m_base_min.cx) lpMMI->ptMinTrackSize.x = m_base_min.cx;
	if (lpMMI->ptMinTrackSize.y < m_base_min.cy) lpMMI->ptMinTrackSize.y = m_base_min.cy;
}

void NSFInfoDialog::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_base_size.cx;
	int dy = cy - m_base_size.cy;
	for (int i=0; i<CHILDREN; ++i)
	{
		CWnd* w = GetDlgItem(m_child[i]);
		CRect r = m_child_rect[i];
		int z = m_child_resize[i];
		if (z & RESIZE_MX) r.OffsetRect(dx,0);
		if (z & RESIZE_MY) r.OffsetRect(0,dy);
		if (z & RESIZE_SX) r.right += dx;
		if (z & RESIZE_SY) r.bottom += dy;
		w->MoveWindow(r,0);
	}
	RedrawWindow();
}

void NSFInfoDialog::OnDelall()
{
  if(ntag.ClearTag()==0)
    MessageBox("���̃^�O���͏����ł��܂���");
}

void NSFInfoDialog::OnDelone()
{
  if(ntag.InitTagItem(m_song-1, CONFIG["TITLE_FORMAT"])==0)
    MessageBox("���̃^�O���͏����ł��܂���");
}

void NSFInfoDialog::OnReadtag()
{
  CMenu *menu = GetMenu();
  CONFIG["READ_TAGINFO"] = !(int)CONFIG["READ_TAGINFO"];
  if(menu)
    menu->CheckMenuItem(ID_READTAG, CONFIG["READ_TAGINFO"]?MF_CHECKED:MF_UNCHECKED);
}

void NSFInfoDialog::OnWritetag()
{
  CMenu *menu = GetMenu();
  CONFIG["WRITE_TAGINFO"] = !(int)CONFIG["WRITE_TAGINFO"];
  if(menu)
  {
    menu->CheckMenuItem(ID_WRITETAG, CONFIG["WRITE_TAGINFO"]?MF_CHECKED:MF_UNCHECKED);
    menu->EnableMenuItem(ID_DELONE, ntag.IsWriteEnable()&&!local_tag?MF_ENABLED:MF_GRAYED);
    menu->EnableMenuItem(ID_DELALL, ntag.IsWriteEnable()&&!local_tag?MF_ENABLED:MF_GRAYED);
  }
}

void NSFInfoDialog::OnTrkinfo()
{
  parent->OpenDialog(NSFplug_UI::DLG_TRACK);
}


void NSFInfoDialog::OnStnClickedArtist()
{
  // TODO : �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
}

void NSFInfoDialog::OnDropFiles(HDROP hDropInfo)
{
  UINT nCount, wSize;
  int nSize;
  CArray <char,int> aryFile;
  CArray <wchar_t, int> w_aryFile;

  if(parent->wa2mod)
  {
    parent->wa2mod->ClearList();
    nCount = DragQueryFileW(hDropInfo, -1, NULL, 0);
    for(UINT i=0;i<nCount;i++)
    {
      wSize = DragQueryFileW(hDropInfo, i, NULL, 0);
      w_aryFile.SetSize(wSize+1);
      DragQueryFileW(hDropInfo, i, w_aryFile.GetData(), wSize+1);
      nSize = file_utf8(w_aryFile.GetData(),NULL,0);
      if (nSize < 0) nSize = 1;
      aryFile.SetSize(nSize);
      file_utf8(w_aryFile.GetData(),aryFile.GetData(),nSize);
      parent->wa2mod->QueueFile(aryFile.GetData());
    }
    parent->wa2mod->PlayStart();
  }

  __super::OnDropFiles(hDropInfo);
}


void NSFInfoDialog::OnLoadpreset()
{
  parent->preset->ShowWindow(SW_SHOW);
}
