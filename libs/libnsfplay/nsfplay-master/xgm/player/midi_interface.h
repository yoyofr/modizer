#ifndef _MIDI_INTERFACE_H_
#define _MIDI_INTERFACE_H_

/** �W���I��MIDI�����̃C���^�t�F�[�X */
class MIDIDeviceI
{
public:
  /**
   * �m�[�gON velocity��0�̎��͏���
   * MIDI���b�Z�[�W�� $9x, $note, $velocity
   */
  virtual void NoteOn(int ch, int note, int velocity)=0;
  /**
   * �m�[�gOFF velocity�͉��������鑬��
   * MIDI���b�Z�[�W�� $8x, $note, $velocity
   */
  virtual void NoteOff(int ch, int note, int velocity)=0;
  
  /**
   * �|���t�H�j�b�N�L�[�v���b�V��
   * MIDI���b�Z�[�W�� $Ax, $pressure
   */
  virtual void PolyKeyPressure(int ch, int note, int pressure)=0;

  /**
   * �`�����l���v���b�V��
   * MIDI���b�Z�[�W�� $Dx, $pressure
   */
  virtual void ChannelPressure(int ch, int pressure)=0;

  /**
   * �s�b�`�x���h
   * MIDI���b�Z�[�W�� $Ex, $data
   */
  virtual void PitchBendChange(int ch, int data)=0;
  
  /**
   * �R���g���[���`�F���W
   * MIDI���b�Z�[�W�� $Bx, $ctrl_no, $data
   */
  virtual void ControlChange(int ch, int ctrl_no, int data)=0;

  /**
   * �v���O�����`�F���W
   * MIDI���b�Z�[�W�� $Cx, $prg_no
   */
  virtual void ProgramChange(int ch, int prg_no)=0;

  /**
   * ���[�h���b�Z�[�W
   * MIDI���b�Z�[�W�� $Bn, $mode, $data   $mode �� $78�`$7f
   */
  virtual void ModeMessage(int ch, int mode, int data)=0;

  /**
   * �G�N�X�N���[�V�u���b�Z�[�W
   * MIDI ���b�Z�[�W�� $F0, $id, $data, ... $F7
   */
  virtual void ExclusiveMessage(int id, int *data, int size)=0;
};




#endif