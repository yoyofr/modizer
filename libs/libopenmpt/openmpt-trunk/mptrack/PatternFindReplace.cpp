/*
 * PatternFindReplace.cpp
 * ----------------------
 * Purpose: Implementation of the pattern search.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "PatternFindReplace.h"
#include "Moddoc.h"
#include "PatternEditorDialogs.h"
#include "PatternFindReplaceDlg.h"
#include "Reporting.h"
#include "resource.h"
#include "View_pat.h"
#include "WindowMessages.h"
#include "../soundlib/mod_specifications.h"

OPENMPT_NAMESPACE_BEGIN

FindReplace FindReplace::instance;


void CViewPattern::OnEditFind()
{
	static bool dialogOpen = false;
	CModDoc *pModDoc = GetDocument();
	if (pModDoc && !dialogOpen)
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		FindReplace settings = FindReplace::instance;
		ModCommand m{};
		if(m_Selection.GetUpperLeft() != m_Selection.GetLowerRight())
		{
			settings.findFlags.set(FindReplace::InPatSelection);
			settings.findFlags.reset(FindReplace::FullSearch);
		} else if(sndFile.Patterns.IsValidPat(m_nPattern))
		{
			const CPattern &pat = sndFile.Patterns[m_nPattern];
			m_Cursor.Sanitize(pat.GetNumRows(), pat.GetNumChannels());
			m = *pat.GetpModCommand(m_Cursor.GetRow(), m_Cursor.GetChannel());
		}

		CFindReplaceTab pageFind(IDD_EDIT_FIND, false, sndFile, settings, m);
		CFindReplaceTab pageReplace(IDD_EDIT_REPLACE, true, sndFile, settings, m);
		CPropertySheet dlg(_T("Find/Replace"));

		dlg.AddPage(&pageFind);
		dlg.AddPage(&pageReplace);
		dialogOpen = true;
		if(dlg.DoModal() == IDOK)
		{
			FindReplace::instance = settings;
			FindReplace::instance.selection = m_Selection;
			m_bContinueSearch = false;
			OnEditFindNext();
		}
		dialogOpen = false;
	}
}


void CViewPattern::OnEditFindNext()
{
	CSoundFile &sndFile = *GetSoundFile();
	const CModSpecifications &specs = sndFile.GetModSpecifications();
	uint32 nFound = 0;

	if(!FindReplace::instance.findFlags[~FindReplace::FullSearch])
	{
		PostMessage(WM_COMMAND, ID_EDIT_FIND);
		return;
	}
	BeginWaitCursor();

	EffectInfo effectInfo(sndFile);

	PATTERNINDEX patStart = m_nPattern;
	PATTERNINDEX patEnd = m_nPattern + 1;

	if(FindReplace::instance.findFlags[FindReplace::FullSearch])
	{
		patStart = 0;
		patEnd = sndFile.Patterns.Size();
	} else if(FindReplace::instance.findFlags[FindReplace::InPatSelection])
	{
		patStart = m_nPattern;
		patEnd = patStart + 1;
	}

	if(m_bContinueSearch)
	{
		patStart = m_nPattern;
	}

	// Do we search for an extended effect?
	bool isExtendedEffect = false;
	if(FindReplace::instance.findFlags[FindReplace::Command])
	{
		UINT fxndx = effectInfo.GetIndexFromEffect(FindReplace::instance.findCommand, static_cast<ModCommand::PARAM>(FindReplace::instance.findParamMin));
		isExtendedEffect = effectInfo.IsExtendedEffect(fxndx);
	}

	CHANNELINDEX firstChannel = 0;
	CHANNELINDEX lastChannel = sndFile.GetNumChannels() - 1;

	if(FindReplace::instance.findFlags[FindReplace::InChannels])
	{
		// Limit search to given channels
		firstChannel = std::min(FindReplace::instance.findChnMin, lastChannel);
		lastChannel = std::min(FindReplace::instance.findChnMax, lastChannel);
	}

	if(FindReplace::instance.findFlags[FindReplace::InPatSelection])
	{
		// Limit search to pattern selection
		firstChannel = std::min(FindReplace::instance.selection.GetStartChannel(), lastChannel);
		lastChannel = std::min(FindReplace::instance.selection.GetEndChannel(), lastChannel);
	}

	for(PATTERNINDEX pat = patStart; pat < patEnd; pat++)
	{
		if(!sndFile.Patterns.IsValidPat(pat))
		{
			continue;
		}

		ROWINDEX row = 0;
		CHANNELINDEX chn = firstChannel;
		if(m_bContinueSearch && pat == patStart && pat == m_nPattern)
		{
			// Continue search from cursor position
			row = GetCurrentRow();
			chn = GetCurrentChannel() + 1;
			if(chn > lastChannel)
			{
				row++;
				chn = firstChannel;
			} else if(chn < firstChannel)
			{
				chn = firstChannel;
			}
		}

		bool firstInPat = true;
		const ROWINDEX numRows = sndFile.Patterns[pat].GetNumRows();
		std::vector<ModCommand::INSTR> lastInstr(sndFile.GetNumChannels(), 0);

		for(; row < numRows; row++)
		{
			ModCommand *m = sndFile.Patterns[pat].GetpModCommand(row, chn);

			for(; chn <= lastChannel; chn++, m++)
			{
				RowMask findWhere;

				if(FindReplace::instance.findFlags[FindReplace::InPatSelection])
				{
					// Limit search to pattern selection
					if((chn == FindReplace::instance.selection.GetStartChannel() || chn == FindReplace::instance.selection.GetEndChannel())
						&& row >= FindReplace::instance.selection.GetStartRow() && row <= FindReplace::instance.selection.GetEndRow())
					{
						// For channels that are on the left and right boundaries of the selection, we need to check
						// columns are actually selected a bit more thoroughly.
						for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
						{
							PatternCursor cursor(row, chn, static_cast<PatternCursor::Columns>(i));
							if(!FindReplace::instance.selection.Contains(cursor))
							{
								switch(i)
								{
								case PatternCursor::noteColumn:		findWhere.note = false; break;
								case PatternCursor::instrColumn:	findWhere.instrument = false; break;
								case PatternCursor::volumeColumn:	findWhere.volume = false; break;
								case PatternCursor::effectColumn:	findWhere.command = false; break;
								case PatternCursor::paramColumn:	findWhere.parameter = false; break;
								}
							}
						}
					} else
					{
						// For channels inside the selection, we have an easier job to solve.
						if(!FindReplace::instance.selection.Contains(PatternCursor(row, chn)))
						{
							findWhere.Clear();
						}
					}
				}

				if(m->instr > 0)
					lastInstr[chn] = m->instr;

				if((FindReplace::instance.findFlags[FindReplace::Note] && (!findWhere.note || m->note < FindReplace::instance.findNoteMin || m->note > FindReplace::instance.findNoteMax))
					|| (FindReplace::instance.findFlags[FindReplace::Instr] && (!findWhere.instrument || m->instr < FindReplace::instance.findInstrMin || m->instr > FindReplace::instance.findInstrMax)))
				{
					continue;
				}

				if(!m->IsPcNote())
				{
					if((FindReplace::instance.findFlags[FindReplace::VolCmd] && (!findWhere.volume || m->volcmd != FindReplace::instance.findVolCmd))
						|| (FindReplace::instance.findFlags[FindReplace::Volume] && (!findWhere.volume || m->volcmd == VOLCMD_NONE || m->vol < FindReplace::instance.findVolumeMin || m->vol > FindReplace::instance.findVolumeMax))
						|| (FindReplace::instance.findFlags[FindReplace::Command] && (!findWhere.command || m->command != FindReplace::instance.findCommand))
						|| (FindReplace::instance.findFlags[FindReplace::Param] && (!findWhere.parameter || m->command == CMD_NONE ||  m->param < FindReplace::instance.findParamMin || m->param > FindReplace::instance.findParamMax))
						|| FindReplace::instance.findFlags[FindReplace::PCParam]
						|| FindReplace::instance.findFlags[FindReplace::PCValue])
					{
						continue;
					}
				} else
				{
					if((FindReplace::instance.findFlags[FindReplace::PCParam] && (!findWhere.volume || m->GetValueVolCol() < FindReplace::instance.findParamMin || m->GetValueVolCol() > FindReplace::instance.findParamMax))
						|| (FindReplace::instance.findFlags[FindReplace::PCValue] && (!(findWhere.command || findWhere.parameter) || m->GetValueEffectCol() < FindReplace::instance.findVolumeMin || m->GetValueEffectCol() > FindReplace::instance.findVolumeMax))
						|| FindReplace::instance.findFlags[FindReplace::VolCmd]
						|| FindReplace::instance.findFlags[FindReplace::Volume]
						|| FindReplace::instance.findFlags[FindReplace::Command]
						|| FindReplace::instance.findFlags[FindReplace::Param])
					{
						continue;
					}
				}

				if((FindReplace::instance.findFlags & (FindReplace::Command | FindReplace::Param)) == FindReplace::Command && isExtendedEffect)
				{
					if((m->param & 0xF0) != (FindReplace::instance.findParamMin & 0xF0))
						continue;
				}

				// Found!
				
				// Do we want to jump to the finding in this pattern?
				const bool updatePos = !FindReplace::instance.replaceFlags.test_all(FindReplace::ReplaceAll | FindReplace::Replace);
				nFound++;

				if(updatePos)
				{
					if(IsLiveRecord())
					{
						// turn off "follow song"
						m_Status.reset(psFollowSong);
						SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 0);
					}

					// Find sequence and order where this pattern is used
					const auto numSequences = sndFile.Order.GetNumSequences();
					auto seq = sndFile.Order.GetCurrentSequenceIndex();
					for(SEQUENCEINDEX i = 0; i < numSequences; i++)
					{
						const bool isCurrentSeq = (i == 0);
						ORDERINDEX matchingOrder = sndFile.Order(seq).FindOrder(pat, isCurrentSeq ? GetCurrentOrder() : 0);
						if(matchingOrder != ORDERINDEX_INVALID)
						{
							if(!isCurrentSeq)
								SendCtrlMessage(CTRLMSG_PAT_SETSEQUENCE, seq);
							SetCurrentOrder(matchingOrder);
							break;
						}
						if(++seq >= numSequences)
							seq = 0;
					}
					// go to place of finding
					SetCurrentPattern(pat);
				}

				PatternCursor::Columns foundCol = PatternCursor::firstColumn;
				if(FindReplace::instance.findFlags[FindReplace::Note])
					foundCol = PatternCursor::noteColumn;
				else if(FindReplace::instance.findFlags[FindReplace::Instr])
					foundCol = PatternCursor::instrColumn;
				else if(FindReplace::instance.findFlags[FindReplace::VolCmd | FindReplace::Volume | FindReplace::PCParam])
					foundCol = PatternCursor::volumeColumn;
				else if(FindReplace::instance.findFlags[FindReplace::Command | FindReplace::PCValue])
					foundCol = PatternCursor::effectColumn;
				else if(FindReplace::instance.findFlags[FindReplace::Param])
					foundCol = PatternCursor::paramColumn;

				if(updatePos)
				{
					// Jump to pattern cell
					SetCursorPosition(PatternCursor(row, chn, foundCol));
				}

				if(!FindReplace::instance.replaceFlags[FindReplace::Replace]) goto EndSearch;

				bool replace = true;

				if(!FindReplace::instance.replaceFlags[FindReplace::ReplaceAll])
				{
					ConfirmAnswer result = Reporting::Confirm("Replace this occurrence?", "Replace", true);
					if(result == cnfCancel)
					{
						goto EndSearch;	// Yuck!
					} else
					{
						replace = (result == cnfYes);
					}
				}
				if(replace)
				{
					if(FindReplace::instance.replaceFlags[FindReplace::ReplaceAll])
					{
						// Just create one logic undo step per pattern when auto-replacing all occurences.
						if(firstInPat)
						{
							GetDocument()->GetPatternUndo().PrepareUndo(pat, firstChannel, row, lastChannel - firstChannel + 1, numRows - row + 1, "Find / Replace", (nFound > 1));
							firstInPat = false;
						}
					} else
					{
						// Create separately undo-able items when replacing manually.
						GetDocument()->GetPatternUndo().PrepareUndo(pat, chn, row, 1, 1, "Find / Replace");
					}

					if(FindReplace::instance.replaceFlags[FindReplace::Instr])
					{
						int instrReplace = FindReplace::instance.replaceInstr;
						int instr = m->instr;
						if(FindReplace::instance.replaceInstrAction == FindReplace::ReplaceRelative && instr > 0)
							instr += instrReplace;
						else if(FindReplace::instance.replaceInstrAction == FindReplace::ReplaceValue)
							instr = instrReplace;

						m->instr = mpt::saturate_cast<ModCommand::INSTR>(instr);
						if(m->instr > 0)
							lastInstr[chn] = m->instr;
					}

					if(FindReplace::instance.replaceFlags[FindReplace::Note])
					{
						int noteReplace = FindReplace::instance.replaceNote;
						if(FindReplace::instance.replaceNoteAction == FindReplace::ReplaceRelative && m->IsNote())
						{
							if(noteReplace == FindReplace::ReplaceOctaveUp || noteReplace == FindReplace::ReplaceOctaveDown)
							{
								noteReplace = GetDocument()->GetInstrumentGroupSize(lastInstr[chn]) * mpt::signum(noteReplace);
							}
							int note = Clamp(m->note + noteReplace, specs.noteMin, specs.noteMax);
							m->note = static_cast<ModCommand::NOTE>(note);
						} else if(FindReplace::instance.replaceNoteAction == FindReplace::ReplaceValue)
						{
							// Replace with another note
							// If we're going to remove a PC Note or replace a normal note by a PC note, wipe out the complete column.
							if(m->IsPcNote() != ModCommand::IsPcNote(static_cast<ModCommand::NOTE>(noteReplace)))
							{
								m->Clear();
							}
							m->note = static_cast<ModCommand::NOTE>(noteReplace);
						}
					}

					bool hadVolume = (m->volcmd == VOLCMD_VOLUME);
					if(FindReplace::instance.replaceFlags[FindReplace::VolCmd])
					{
						m->volcmd = FindReplace::instance.replaceVolCmd;
					}

					if(FindReplace::instance.replaceFlags[FindReplace::Volume])
					{
						int volReplace = FindReplace::instance.replaceVolume;
						int vol = m->vol;
						if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceRelative || FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceMultiply)
						{
							if(!hadVolume && m->volcmd == VOLCMD_VOLUME)
								vol = GetDefaultVolume(*m, lastInstr[chn]);

							if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceRelative)
								vol += volReplace;
							else
								vol = Util::muldivr(vol, volReplace, 100);
						} else if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceValue)
						{
							vol = volReplace;
						}
						m->vol = mpt::saturate_cast<ModCommand::VOL>(vol);
					}

					if(FindReplace::instance.replaceFlags[FindReplace::VolCmd | FindReplace::Volume] && m->volcmd != VOLCMD_NONE)
					{
						// Fix volume command parameters if necessary. This is necesary e.g.
						// when there was a command "v24" and the user searched for v and replaced it by d.
						// In that case, d24 wouldn't be a valid command.
						ModCommand::VOL minVal = 0, maxVal = 64;
						if(effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m->volcmd), nullptr, &minVal, &maxVal))
						{
							Limit(m->vol, minVal, maxVal);
						}
					}

					hadVolume = (m->command == CMD_VOLUME);
					if(FindReplace::instance.replaceFlags[FindReplace::Command])
					{
						m->command = FindReplace::instance.replaceCommand;
					}

					if(FindReplace::instance.replaceFlags[FindReplace::Param])
					{
						int paramReplace = FindReplace::instance.replaceParam;
						int param = m->param;
						if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceRelative || FindReplace::instance.replaceParamAction == FindReplace::ReplaceMultiply)
						{
							if(isExtendedEffect)
								param &= 0x0F;

							if(!hadVolume && m->command == CMD_VOLUME)
								param = GetDefaultVolume(*m, lastInstr[chn]);

							if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceRelative)
								param += paramReplace;
							else
								param = Util::muldivr(param, paramReplace, 100);

							if(isExtendedEffect)
								param = Clamp(param, 0, 15) | (m->param & 0xF0);
						} else if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceValue)
						{
							param = paramReplace;
						}
						
						if(isExtendedEffect && !FindReplace::instance.replaceFlags[FindReplace::Command])
							m->param = static_cast<ModCommand::PARAM>((m->param & 0xF0) | (param & 0x0F));
						else
							m->param = mpt::saturate_cast<ModCommand::PARAM>(param);
					}

					if(FindReplace::instance.replaceFlags[FindReplace::PCParam])
					{
						int paramReplace = FindReplace::instance.replaceParam;
						int param = m->GetValueVolCol();
						if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceRelative)
							param += paramReplace;
						else if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceMultiply)
							param = Util::muldivr(param, paramReplace, 100);
						else if(FindReplace::instance.replaceParamAction == FindReplace::ReplaceValue)
							param = paramReplace;

						m->SetValueVolCol(static_cast<uint16>(Clamp(param, 0, ModCommand::maxColumnValue)));
					}

					if(FindReplace::instance.replaceFlags[FindReplace::PCValue])
					{
						int valueReplace = FindReplace::instance.replaceVolume;
						int value = m->GetValueEffectCol();
						if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceRelative)
							value += valueReplace;
						else if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceMultiply)
							value = Util::muldivr(value, valueReplace, 100);
						else if(FindReplace::instance.replaceVolumeAction == FindReplace::ReplaceValue)
							value = valueReplace;

						m->SetValueEffectCol(static_cast<uint16>(Clamp(value, 0, ModCommand::maxColumnValue)));
					}

					SetModified(false);
					if(updatePos)
						InvalidateRow();
				}
			}
			chn = firstChannel;
		}

	}
EndSearch:

	if(FindReplace::instance.replaceFlags[FindReplace::ReplaceAll])
	{
		InvalidatePattern();
	}

	if(FindReplace::instance.findFlags[FindReplace::InPatSelection] && (nFound == 0 || (FindReplace::instance.replaceFlags & (FindReplace::Replace | FindReplace::ReplaceAll)) == FindReplace::Replace))
	{
		// Restore original selection if we didn't find anything or just replaced stuff manually.
		m_Selection = FindReplace::instance.selection;
		InvalidatePattern();
	}

	m_bContinueSearch = true;

	EndWaitCursor();

	// Display search results
	if(nFound == 0)
	{
		CString result;
		result.Preallocate(14 + 16);
		result = _T("Cannot find \"");

		// Note
		if(FindReplace::instance.findFlags[FindReplace::Note])
		{
			result += mpt::ToCString(sndFile.GetNoteName(FindReplace::instance.findNoteMin));
			if(FindReplace::instance.findNoteMax > FindReplace::instance.findNoteMin)
			{
				result.AppendChar(_T('-'));
				result += mpt::ToCString(sndFile.GetNoteName(FindReplace::instance.findNoteMax));
			}
		} else
		{
			result += _T("???");
		}
		result.AppendChar(_T(' '));

		// Instrument
		if(FindReplace::instance.findFlags[FindReplace::Instr])
		{
			if(FindReplace::instance.findInstrMin)
				result.AppendFormat(_T("%03d"), FindReplace::instance.findInstrMin);
			else
				result.Append(_T(" .."));
			if(FindReplace::instance.findInstrMax > FindReplace::instance.findInstrMin)
				result.AppendFormat(_T("-%03d"), FindReplace::instance.findInstrMax);
		} else
		{
			result.Append(_T(" ??"));
		}
		result.AppendChar(_T(' '));

		// Volume Command
		if(FindReplace::instance.findFlags[FindReplace::VolCmd])
		{
			if(FindReplace::instance.findVolCmd != VOLCMD_NONE)
				result.AppendChar(specs.GetVolEffectLetter(FindReplace::instance.findVolCmd));
			else
				result.AppendChar(_T('.'));
		} else if(FindReplace::instance.findFlags[FindReplace::PCParam])
		{
			result.AppendFormat(_T("%03d"), FindReplace::instance.findParamMin);
			if(FindReplace::instance.findParamMax > FindReplace::instance.findParamMin)
				result.AppendFormat(_T("-%03d"), FindReplace::instance.findParamMax);
		} else
		{
			result.AppendChar(_T('?'));
		}

		// Volume Parameter
		if(FindReplace::instance.findFlags[FindReplace::Volume])
		{
			result.AppendFormat(_T("%02d"), FindReplace::instance.findVolumeMin);
			if(FindReplace::instance.findVolumeMax > FindReplace::instance.findVolumeMin)
				result.AppendFormat(_T("-%02d"), FindReplace::instance.findVolumeMax);
		} else if(!FindReplace::instance.findFlags[FindReplace::PCParam])
		{
			result.Append(_T("??"));
		}
		result.AppendChar(_T(' '));

		// Effect Command
		if(FindReplace::instance.findFlags[FindReplace::Command])
		{
			if(FindReplace::instance.findCommand != CMD_NONE)
				result.AppendChar(specs.GetEffectLetter(FindReplace::instance.findCommand));
			else
				result.AppendChar(_T('.'));
		} else if(FindReplace::instance.findFlags[FindReplace::PCValue])
		{
			result.AppendFormat(_T("%03d"), FindReplace::instance.findVolumeMin);
			if(FindReplace::instance.findVolumeMax > FindReplace::instance.findVolumeMin)
				result.AppendFormat(_T("-%03d"), FindReplace::instance.findVolumeMax);
		} else
		{
			result.AppendChar(_T('?'));
		}

		// Effect Parameter
		if(FindReplace::instance.findFlags[FindReplace::Param])
		{
			result.AppendFormat(_T("%02X"), FindReplace::instance.findParamMin);
			if(FindReplace::instance.findParamMax > FindReplace::instance.findParamMin)
				result.AppendFormat(_T("-%02X"), FindReplace::instance.findParamMax);
		} else if(!FindReplace::instance.findFlags[FindReplace::PCValue])
		{
			result.Append(_T("??"));
		}

		result.AppendChar(_T('"'));

		Reporting::Information(result, _T("Find/Replace"));
	}
}

OPENMPT_NAMESPACE_END
