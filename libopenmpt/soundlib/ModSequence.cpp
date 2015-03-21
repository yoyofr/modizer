/*
 * ModSequence.cpp
 * ---------------
 * Purpose: Order and sequence handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "ModSequence.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Reporting.h"
#endif // MODPLUG_TRACKER
#include "../common/version.h"
#include "../common/serialization_utils.h"
#include "../common/FileReader.h"
#include <functional>

#if defined(MODPLUG_TRACKER)
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

#define str_SequenceTruncationNote (GetStrI18N("Module has sequence of length %1; it will be truncated to maximum supported length, %2."))

ModSequence::ModSequence(CSoundFile &rSf,
						 PATTERNINDEX* pArray,
						 ORDERINDEX nSize,
						 ORDERINDEX nCapacity, 
						 const bool bDeletableArray) :
		m_sndFile(rSf),
		m_pArray(pArray),
		m_nSize(nSize),
		m_nCapacity(nCapacity),
		m_nInvalidIndex(0xFF),
		m_nIgnoreIndex(0xFE),
		m_bDeletableArray(bDeletableArray)
//-------------------------------------------------------
{}


ModSequence::ModSequence(CSoundFile& rSf, ORDERINDEX nSize) :
	m_sndFile(rSf),
	m_nInvalidIndex(GetInvalidPatIndex(MOD_TYPE_MPT)),
	m_nIgnoreIndex(GetIgnoreIndex(MOD_TYPE_MPT)),
	m_bDeletableArray(true)
//-------------------------------------------------------------------
{
	m_nSize = nSize;
	m_nCapacity = m_nSize;
	m_pArray = new PATTERNINDEX[m_nCapacity];
	std::fill(begin(), end(), GetInvalidPatIndex(MOD_TYPE_MPT));
}


ModSequence::ModSequence(const ModSequence& seq) :
	m_sndFile(seq.m_sndFile),
	m_pArray(nullptr),
	m_nSize(0),
	m_nCapacity(0),
	m_nInvalidIndex(0xFF),
	m_nIgnoreIndex(0xFE),
	m_bDeletableArray(false)
//------------------------------------------
{
	*this = seq;
}


bool ModSequence::NeedsExtraDatafield() const
//-------------------------------------------
{
	if(m_sndFile.GetType() == MOD_TYPE_MPT && m_sndFile.Patterns.Size() > 0xFD)
		return true;
	else
		return false;
}

namespace
{
	// Functor for detecting non-valid patterns from sequence.
	struct IsNotValidPat
	{
		IsNotValidPat(const CSoundFile &sf) : sndFile(sf) { }
		bool operator() (PATTERNINDEX i) { return !sndFile.Patterns.IsValidPat(i); }
		const CSoundFile &sndFile;
	};
}


void ModSequence::AdjustToNewModType(const MODTYPE oldtype)
//---------------------------------------------------------
{
	const CModSpecifications specs = m_sndFile.GetModSpecifications();
	const MODTYPE newtype = m_sndFile.GetType();

	m_nInvalidIndex = GetInvalidPatIndex(newtype);
	m_nIgnoreIndex = GetIgnoreIndex(newtype);

	if(oldtype != MOD_TYPE_NONE)
	{
		// If not supported, remove "+++" separator order items.
		if(specs.hasIgnoreIndex == false)
		{
			RemovePattern(GetIgnoreIndex(oldtype));
		} else
		{
			Replace(GetIgnoreIndex(oldtype), GetIgnoreIndex());
		}
		// If not supported, remove "---" items between patterns.
		if(specs.hasStopIndex == false)
		{
			RemovePattern(GetInvalidPatIndex(oldtype));
		}
	}

	//Resize orderlist if needed.
	if(specs.ordersMax < GetLength())
	{
		// Order list too long? -> remove unnecessary order items first.
		if(oldtype != MOD_TYPE_NONE && specs.ordersMax < GetLengthTailTrimmed())
		{
			iterator iter = std::remove_if(begin(), end(), IsNotValidPat(m_sndFile));
			std::fill(iter, end(), GetInvalidPatIndex());
			if(GetLengthTailTrimmed() > specs.ordersMax)
			{
				m_sndFile.AddToLog(mpt::String::Print("WARNING: Order list has been trimmed!"));
			}
		}
		resize(specs.ordersMax);
	}
		
	// Replace items used to denote end of song order.
	if(oldtype != MOD_TYPE_NONE) Replace(GetInvalidPatIndex(oldtype), GetInvalidPatIndex());
}


ORDERINDEX ModSequence::GetLengthTailTrimmed() const
//--------------------------------------------------
{
	ORDERINDEX nEnd = GetLength();
	if(nEnd == 0) return 0;
	nEnd--;
	const PATTERNINDEX iInvalid = GetInvalidPatIndex();
	while(nEnd > 0 && (*this)[nEnd] == iInvalid)
		nEnd--;
	return ((*this)[nEnd] == iInvalid) ? 0 : nEnd+1;
}


ORDERINDEX ModSequence::GetLengthFirstEmpty() const
//-------------------------------------------------
{
	return static_cast<ORDERINDEX>(std::find(begin(), end(), GetInvalidPatIndex()) - begin());
}


ORDERINDEX ModSequence::GetNextOrderIgnoringSkips(const ORDERINDEX start) const
//-----------------------------------------------------------------------------
{
	const ORDERINDEX nLength = GetLength();
	if(nLength == 0) return 0;
	ORDERINDEX next = std::min(ORDERINDEX(nLength - 1), ORDERINDEX(start + 1));
	while(next+1 < nLength && (*this)[next] == GetIgnoreIndex()) next++;
	return next;
}


ORDERINDEX ModSequence::GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const
//---------------------------------------------------------------------------------
{
	const ORDERINDEX nLength = GetLength();
	if(start == 0 || nLength == 0) return 0;
	ORDERINDEX prev = std::min(ORDERINDEX(start - 1), ORDERINDEX(nLength - 1));
	while(prev > 0 && (*this)[prev] == GetIgnoreIndex()) prev--;
	return prev;
}


void ModSequence::Init()
//----------------------
{
	resize(MAX_ORDERS);
	std::fill(begin(), end(), GetInvalidPatIndex());
}


void ModSequence::Remove(ORDERINDEX nPosBegin, ORDERINDEX nPosEnd)
//----------------------------------------------------------------
{
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	if (nPosEnd < nPosBegin || nPosEnd >= nLengthTt)
		return;
	const ORDERINDEX nMoveCount = nLengthTt - (nPosEnd + 1);
	// Move orders left.
	if (nMoveCount > 0)
		memmove(m_pArray + nPosBegin, m_pArray + nPosEnd + 1, sizeof(PATTERNINDEX) * nMoveCount);
	// Clear tail orders.
	std::fill(m_pArray + nPosBegin + nMoveCount, m_pArray + nLengthTt, GetInvalidPatIndex());
}


// Functor for fixing jump commands to moved order items
struct FixJumpCommands
//====================
{
	FixJumpCommands(const std::vector<ORDERINDEX> &offsets) : jumpOffset(offsets) {};

	void operator()(ModCommand& m)
	{
		if(m.command == CMD_POSITIONJUMP && m.param < jumpOffset.size())
		{
			m.param = ModCommand::PARAM(int(m.param) - jumpOffset[m.param]);
		}
	}

	const std::vector<ORDERINDEX> jumpOffset;
};


// Remove all references to a given pattern index from the order list. Jump commands are updated accordingly.
void ModSequence::RemovePattern(PATTERNINDEX which)
//-------------------------------------------------
{
	const ORDERINDEX orderLength = GetLengthTailTrimmed();
	ORDERINDEX currentLength = orderLength;

	// Associate order item index with jump offset (i.e. how much it moved forwards)
	std::vector<ORDERINDEX> jumpOffset(orderLength, 0);
	ORDERINDEX maxJump = 0;

	for(ORDERINDEX i = 0; i < currentLength; i++)
	{
		if(At(i) == which)
		{
			maxJump++;
			// Move order list forwards, update jump counters
			for(ORDERINDEX j = i + 1; j < orderLength; j++)
			{
				At(j - 1) = At(j);
				jumpOffset[j] = maxJump;
			}
			At(--currentLength) = GetInvalidPatIndex();
		}
	}

	m_sndFile.Patterns.ForEachModCommand(FixJumpCommands(jumpOffset));
	if(m_sndFile.m_nRestartPos < jumpOffset.size())
	{
		m_sndFile.m_nRestartPos -= jumpOffset[m_sndFile.m_nRestartPos];
	}
}


ORDERINDEX ModSequence::Insert(ORDERINDEX nPos, ORDERINDEX nCount, PATTERNINDEX nFill)
//------------------------------------------------------------------------------------
{
	if (nPos >= m_sndFile.GetModSpecifications().ordersMax || nCount == 0)
		return 0;
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	// Limit number of orders to be inserted.
	LimitMax(nCount, ORDERINDEX(m_sndFile.GetModSpecifications().ordersMax - nPos));
	// Calculate new length.
	const ORDERINDEX nNewLength = std::min<ORDERINDEX>(nLengthTt + nCount, m_sndFile.GetModSpecifications().ordersMax);
	// Resize if needed.
	if (nNewLength > GetLength())
		resize(nNewLength);
	// Calculate how many orders would need to be moved(nNeededSpace) and how many can actually
	// be moved(nFreeSpace).
	const ORDERINDEX nNeededSpace = nLengthTt - nPos;
	const ORDERINDEX nFreeSpace = GetLength() - (nPos + nCount);
	// Move orders nCount steps right starting from nPos.
	if (nPos < nLengthTt)
		memmove(m_pArray + nPos + nCount, m_pArray + nPos, MIN(nFreeSpace, nNeededSpace) * sizeof(PATTERNINDEX));
	// Set nFill to new orders.
	std::fill(begin() + nPos, begin() + nPos + nCount, nFill);
	return nCount;
}


void ModSequence::Append(PATTERNINDEX nPat)
//-----------------------------------------
{
	resize(m_nSize + 1, nPat);
}


void ModSequence::resize(ORDERINDEX nNewSize, PATTERNINDEX nFill)
//---------------------------------------------------------------
{
	if (nNewSize == m_nSize) return;
	if (nNewSize <= m_nCapacity)
	{
		if (nNewSize > m_nSize)
			std::fill(begin() + m_nSize, begin() + nNewSize, nFill);
		m_nSize = nNewSize;
	}
	else
	{
		const PATTERNINDEX* const pOld = m_pArray;
		m_nCapacity = nNewSize + 100;
		m_pArray = new PATTERNINDEX[m_nCapacity];
		std::copy(pOld, pOld+m_nSize, m_pArray);
		std::fill(m_pArray + m_nSize, m_pArray + nNewSize, nFill);
		m_nSize = nNewSize;
		if (m_bDeletableArray)
			delete[] pOld;
		m_bDeletableArray = true;
	}
}


void ModSequence::clear()
//-----------------------
{
	m_nSize = 0;
}


ModSequence& ModSequence::operator=(const ModSequence& seq)
//---------------------------------------------------------
{
	if (&seq == this)
		return *this;
	m_nIgnoreIndex = seq.m_nIgnoreIndex;
	m_nInvalidIndex = seq.m_nInvalidIndex;
	resize(seq.GetLength());
	std::copy(seq.begin(), seq.end(), begin());
	m_sName = seq.m_sName;
	return *this;
}


ORDERINDEX ModSequence::FindOrder(PATTERNINDEX nPat, ORDERINDEX startFromOrder, bool searchForward) const
//-------------------------------------------------------------------------------------------------------
{
	const ORDERINDEX maxOrder = GetLength();
	ORDERINDEX foundAtOrder = ORDERINDEX_INVALID;
	ORDERINDEX candidateOrder = 0;

	for (ORDERINDEX p = 0; p < maxOrder; p++)
	{
		if (searchForward)
		{
			candidateOrder = (startFromOrder + p) % maxOrder;				// wrap around MAX_ORDERS
		} else
		{
			candidateOrder = (startFromOrder - p + maxOrder) % maxOrder;	// wrap around 0 and MAX_ORDERS
		}
		if ((*this)[candidateOrder] == nPat)
		{
			foundAtOrder = candidateOrder;
			break;
		}
	}

	return foundAtOrder;
}


/////////////////////////////////////
// ModSequenceSet
/////////////////////////////////////


ModSequenceSet::ModSequenceSet(CSoundFile& sndFile)
	: ModSequence(sndFile, m_Cache, s_nCacheSize, s_nCacheSize, false),
	  m_nCurrentSeq(0)
//--------------------------------------------------------------
{
	std::fill(m_Cache, m_Cache + s_nCacheSize, GetInvalidPatIndex(MOD_TYPE_MPT));
	m_Sequences.push_back(ModSequence(sndFile, s_nCacheSize));
}


const ModSequence& ModSequenceSet::GetSequence(SEQUENCEINDEX nSeq) const
//----------------------------------------------------------------------
{
	if(nSeq == GetCurrentSequenceIndex())
		return *this;
	else
		return m_Sequences[nSeq];
}


ModSequence& ModSequenceSet::GetSequence(SEQUENCEINDEX nSeq)
//----------------------------------------------------------
{
	if(nSeq == GetCurrentSequenceIndex())
		return *this;
	else
		return m_Sequences[nSeq];
}


void ModSequenceSet::CopyCacheToStorage()
//---------------------------------------
{
	m_Sequences[m_nCurrentSeq] = *this;
}


void ModSequenceSet::CopyStorageToCache()
//---------------------------------------
{
	const ModSequence& rSeq = m_Sequences[m_nCurrentSeq];
	if(rSeq.GetLength() <= s_nCacheSize)
	{
		PATTERNINDEX* pOld = m_pArray;
		m_pArray = m_Cache;
		m_nSize = rSeq.GetLength();
		m_nCapacity = s_nCacheSize;
		m_sName = rSeq.m_sName;
		std::copy(rSeq.m_pArray, rSeq.m_pArray+m_nSize, m_pArray);
		if (m_bDeletableArray)
			delete[] pOld;
		m_bDeletableArray = false;
	}
	else
		ModSequence::operator=(rSeq);
}


void ModSequenceSet::SetSequence(SEQUENCEINDEX n)
//-----------------------------------------------
{
	CopyCacheToStorage();
	m_nCurrentSeq = n;
	CopyStorageToCache();
}


SEQUENCEINDEX ModSequenceSet::AddSequence(bool bDuplicate)
//--------------------------------------------------------
{
	if(GetNumSequences() == MAX_SEQUENCES)
		return SEQUENCEINDEX_INVALID;
	m_Sequences.push_back(ModSequence(m_sndFile, s_nCacheSize)); 
	if (bDuplicate)
	{
		m_Sequences.back() = *this;
		m_Sequences.back().m_sName = ""; // Don't copy sequence name.
	}
	SetSequence(GetNumSequences() - 1);
	return GetNumSequences() - 1;
}


void ModSequenceSet::RemoveSequence(SEQUENCEINDEX i)
//--------------------------------------------------
{
	// Do nothing if index is invalid or if there's only one sequence left.
	if(i >= m_Sequences.size() || m_Sequences.size() <= 1) 
		return;
	const bool bSequenceChanges = (i == m_nCurrentSeq);
	m_Sequences.erase(m_Sequences.begin() + i);
	if(i < m_nCurrentSeq || m_nCurrentSeq >= GetNumSequences())
		m_nCurrentSeq--;
	if(bSequenceChanges)
		CopyStorageToCache();
}


void ModSequenceSet::OnModTypeChanged(const MODTYPE oldtype)
//----------------------------------------------------------
{
	const SEQUENCEINDEX nSeqs = GetNumSequences();
	for(SEQUENCEINDEX n = 0; n < nSeqs; n++)
	{
		GetSequence(n).AdjustToNewModType(oldtype);
	}
	// Multisequences not suppported by other formats
	if(oldtype != MOD_TYPE_NONE && m_sndFile.GetModSpecifications().sequencesMax <= 1)
		MergeSequences();

	// Convert sequence with separator patterns into multiple sequences?
	if(oldtype != MOD_TYPE_NONE && m_sndFile.GetModSpecifications().sequencesMax > 1 && GetNumSequences() == 1)
		ConvertSubsongsToMultipleSequences();
}


bool ModSequenceSet::ConvertSubsongsToMultipleSequences()
//-------------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	// Allow conversion only if there's only one sequence.
	if(GetNumSequences() != 1 || m_sndFile.GetModSpecifications().sequencesMax <= 1)
		return false;

	bool hasSepPatterns = false;
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	for(ORDERINDEX nOrd = 0; nOrd < nLengthTt; nOrd++)
	{
		if(!m_sndFile.Patterns.IsValidPat(At(nOrd)) && At(nOrd) != GetIgnoreIndex())
		{
			hasSepPatterns = true;
			break;
		}
	}
	bool modified = false;

	if(hasSepPatterns &&
		Reporting::Confirm("The order list contains separator items.\nThe new format supports multiple sequences, do you want to convert those separate tracks into multiple song sequences?",
		"Order list conversion", false, true) == cnfYes)
	{

		SetSequence(0);
		for(ORDERINDEX nOrd = 0; nOrd < GetLengthTailTrimmed(); nOrd++)
		{
			// end of subsong?
			if(!m_sndFile.Patterns.IsValidPat(At(nOrd)) && At(nOrd) != GetIgnoreIndex())
			{
				ORDERINDEX oldLength = GetLengthTailTrimmed();
				// remove all separator patterns between current and next subsong first
				while(nOrd < oldLength && (!m_sndFile.Patterns.IsValidIndex(At(nOrd))))
				{
					At(nOrd) = GetInvalidPatIndex();
					nOrd++;
					modified = true;
				}
				if(nOrd >= oldLength) break;
				ORDERINDEX startOrd = nOrd;
				modified = true;

				SEQUENCEINDEX newSeq = AddSequence(false);
				SetSequence(newSeq);

				// resize new seqeuence if necessary
				if(GetLength() < oldLength - startOrd)
				{
					resize(oldLength - startOrd);
				}

				// now, move all following orders to the new sequence
				while(nOrd < oldLength)
				{
					PATTERNINDEX copyPat = GetSequence(newSeq - 1)[nOrd];
					At(nOrd - startOrd) = copyPat;
					nOrd++;

					// is this a valid pattern? adjust pattern jump commands, if necessary.
					if(m_sndFile.Patterns.IsValidPat(copyPat))
					{
						ModCommand *m = m_sndFile.Patterns[copyPat];
						for(size_t len = m_sndFile.Patterns[copyPat].GetNumRows() * m_sndFile.m_nChannels; len; m++, len--)
						{
							if(m->command == CMD_POSITIONJUMP && m->param >= startOrd)
							{
								m->param = static_cast<ModCommand::PARAM>(m->param - startOrd);
							}
						}
					}
				}
				SetSequence(newSeq - 1);
				Remove(startOrd, oldLength - 1);
				SetSequence(newSeq);
				// start from beginning...
				nOrd = 0;
				if(GetLengthTailTrimmed() == 0 || !m_sndFile.Patterns.IsValidIndex(At(nOrd))) break;
			}
		}
		SetSequence(0);
	}
	return modified;
#else
	return false;
#endif // MODPLUG_TRACKER
}


bool ModSequenceSet::MergeSequences()
//-----------------------------------
{
	if(GetNumSequences() <= 1)
		return false;

	SetSequence(0);
	resize(GetLengthTailTrimmed());
	SEQUENCEINDEX removedSequences = 0; // sequence count
	std::vector <SEQUENCEINDEX> patternsFixed; // pattern fixed by other sequence already?
	patternsFixed.resize(m_sndFile.Patterns.Size(), SEQUENCEINDEX_INVALID);
	// Set up vector
	for(ORDERINDEX nOrd = 0; nOrd < GetLengthTailTrimmed(); nOrd++)
	{
		PATTERNINDEX nPat = At(nOrd);
		if(!m_sndFile.Patterns.IsValidPat(nPat)) continue;
		patternsFixed[nPat] = 0;
	}

	while(GetNumSequences() > 1)
	{
		removedSequences++;
		const ORDERINDEX nFirstOrder = GetLengthTailTrimmed() + 1; // +1 for separator item
		if(nFirstOrder + GetSequence(1).GetLengthTailTrimmed() > m_sndFile.GetModSpecifications().ordersMax)
		{
			m_sndFile.AddToLog(mpt::String::Print("WARNING: Cannot merge Sequence %1 (too long!)", removedSequences));
			RemoveSequence(1);
			continue;
		}
		Append(GetInvalidPatIndex()); // Separator item
		for(ORDERINDEX nOrd = 0; nOrd < GetSequence(1).GetLengthTailTrimmed(); nOrd++)
		{
			PATTERNINDEX nPat = GetSequence(1)[nOrd];
			Append(nPat);

			// Try to fix patterns (Bxx commands)
			if(!m_sndFile.Patterns.IsValidPat(nPat)) continue;

			ModCommand *m = m_sndFile.Patterns[nPat];
			for(size_t len = 0; len < m_sndFile.Patterns[nPat].GetNumRows() * m_sndFile.m_nChannels; m++, len++)
			{
				if(m->command == CMD_POSITIONJUMP)
				{
					if(patternsFixed[nPat] != SEQUENCEINDEX_INVALID && patternsFixed[nPat] != removedSequences)
					{
						// Oops, some other sequence uses this pattern already.
						const PATTERNINDEX nNewPat = m_sndFile.Patterns.Insert(m_sndFile.Patterns[nPat].GetNumRows());
						if(nNewPat != SEQUENCEINDEX_INVALID)
						{
							// could create new pattern - copy data over and continue from here.
							At(nFirstOrder + nOrd) = nNewPat;
							ModCommand *pSrc = m_sndFile.Patterns[nPat];
							ModCommand *pDest = m_sndFile.Patterns[nNewPat];
							memcpy(pDest, pSrc, m_sndFile.Patterns[nPat].GetNumRows() * m_sndFile.m_nChannels * sizeof(ModCommand));
							m = pDest + len;
							patternsFixed.resize(MAX(nNewPat + 1, (PATTERNINDEX)patternsFixed.size()), SEQUENCEINDEX_INVALID);
							nPat = nNewPat;
						} else
						{
							// cannot create new pattern: notify the user
							m_sndFile.AddToLog(mpt::String::Print("CONFLICT: Pattern break commands in Pattern %1 might be broken since it has been used in several sequences!", nPat));
						}
					}
					m->param  = static_cast<ModCommand::PARAM>(m->param + nFirstOrder);
					patternsFixed[nPat] = removedSequences;
				}
			}

		}
		RemoveSequence(1);
	}
	// Remove order name + fill up with empty patterns.
	m_sName = "";
	const ORDERINDEX CacheSize = s_nCacheSize; // work-around reference to static const member problem
	const ORDERINDEX nMinLength = std::min(CacheSize, m_sndFile.GetModSpecifications().ordersMax);
	if(GetLength() < nMinLength)
		resize(nMinLength);
	return true;
}


#ifdef MODPLUG_TRACKER
// Check if a playback position is currently locked (inaccessible)
bool ModSequence::IsPositionLocked(ORDERINDEX position) const
//-----------------------------------------------------------
{
	return(m_sndFile.m_lockOrderStart != ORDERINDEX_INVALID
		&& (position < m_sndFile.m_lockOrderStart || position > m_sndFile.m_lockOrderEnd));
}
#endif // MODPLUG_TRACKER


void ModSequence::SetName(const std::string &newName)
//---------------------------------------------------
{
	m_sName = newName;
}


std::string ModSequence::GetName() const
//--------------------------------------
{
	return m_sName;
}


/////////////////////////////////////
// Read/Write
/////////////////////////////////////


bool ModSequence::Deserialize(FileReader &file)
//---------------------------------------------
{
	if(!file.CanRead(2 + 4)) return false;

	uint16 version = file.ReadUint16LE();
	if(version != 0) return false;

	uint32 s = file.ReadUint32LE();
	if(s > 65000 || !file.CanRead(s * 4)) return false;

	const FileReader::off_t endPos = file.GetPosition() + s * 4;
	LimitMax(s, ModSpecs::mptm.ordersMax);

	resize(static_cast<ORDERINDEX>(s));
	for(size_t i = 0; i < s; i++)
	{
		(*this)[i] = static_cast<PATTERNINDEX>(file.ReadUint32LE());
	}
	file.Seek(endPos);
	return true;
}


size_t ModSequence::WriteAsByte(FILE* f, const uint16 count) const
//----------------------------------------------------------------
{
	const size_t limit = MIN(count, GetLength());

	size_t i = 0;
	
	for(i = 0; i < limit; i++)
	{
		const PATTERNINDEX pat = (*this)[i];
		BYTE temp = static_cast<BYTE>((*this)[i]);

		if(pat > 0xFD)
		{
			if(pat == GetInvalidPatIndex()) temp = 0xFF;
			else temp = 0xFE;
		}
		fwrite(&temp, 1, 1, f);
	}
	// Fill non-existing order items with stop indices
	for(i = limit; i < count; i++)
	{
		BYTE temp = 0xFF;
		fwrite(&temp, 1, 1, f);
	}
	return i; //Returns the number of bytes written.
}


// TODO: Need a way to declare skip/stop indices?
bool ModSequence::ReadAsByte(FileReader &file, size_t howMany, size_t readEntries)
//--------------------------------------------------------------------------------
{
	if(!file.CanRead(howMany))
	{
		return false;
	}
	LimitMax(readEntries, howMany);
	LimitMax(readEntries, ORDERINDEX_MAX);

	if(GetLength() < readEntries)
	{
		resize((ORDERINDEX)readEntries);
	}

	for(size_t i = 0; i < readEntries; i++)
	{
		(*this)[i] = file.ReadUint8();
	}
	std::fill(begin() + readEntries, end(), GetInvalidPatIndex());

	file.Skip(howMany - readEntries);
	return true;
}


MODTYPE ModSequence::GetSndFileType() const
//-----------------------------------------
{
	return m_sndFile.GetType();
}


void ReadModSequenceOld(std::istream& iStrm, ModSequenceSet& seq, const size_t)
//-----------------------------------------------------------------------------
{
	uint16 size;
	mpt::IO::ReadIntLE<uint16>(iStrm, size);
	if(size > ModSpecs::mptm.ordersMax)
	{
		seq.m_sndFile.AddToLog(mpt::String::Print(str_SequenceTruncationNote, size, ModSpecs::mptm.ordersMax));
		size = ModSpecs::mptm.ordersMax;
	}
	seq.resize(MAX(size, MAX_ORDERS));
	if(size == 0)
		{ seq.Init(); return; }

	for(size_t i = 0; i < size; i++)
	{
		uint16 temp;
		mpt::IO::ReadIntLE<uint16>(iStrm, temp);
		seq[i] = temp;
	}
}


void WriteModSequenceOld(std::ostream& oStrm, const ModSequenceSet& seq)
//----------------------------------------------------------------------
{
	const uint16 size = seq.GetLength();
	mpt::IO::WriteIntLE<uint16>(oStrm, size);
	const ModSequenceSet::const_iterator endIter = seq.end();
	for(ModSequenceSet::const_iterator citer = seq.begin(); citer != endIter; citer++)
	{
		const uint16 temp = static_cast<uint16>(*citer);
		mpt::IO::WriteIntLE<uint16>(oStrm, temp);
	}
}


void WriteModSequence(std::ostream& oStrm, const ModSequence& seq)
//----------------------------------------------------------------
{
	srlztn::SsbWrite ssb(oStrm);
	ssb.BeginWrite(FileIdSequence, MptVersion::num);
	ssb.WriteItem(seq.m_sName, "n");
	const uint16 nLength = seq.GetLengthTailTrimmed();
	ssb.WriteItem<uint16>(nLength, "l");
	ssb.WriteItem(seq.m_pArray, "a", srlztn::ArrayWriter<uint16>(nLength));
	ssb.FinishWrite();
}


void ReadModSequence(std::istream& iStrm, ModSequence& seq, const size_t)
//-----------------------------------------------------------------------
{
	srlztn::SsbRead ssb(iStrm);
	ssb.BeginRead(FileIdSequence, MptVersion::num);
	if ((ssb.GetStatus() & srlztn::SNT_FAILURE) != 0)
		return;
	std::string str;
	ssb.ReadItem(str, "n");
	seq.m_sName = str;
	uint16 nSize = MAX_ORDERS;
	ssb.ReadItem<uint16>(nSize, "l");
	LimitMax(nSize, ModSpecs::mptm.ordersMax);
	seq.resize(MAX(nSize, ModSequenceSet::s_nCacheSize));
	ssb.ReadItem(seq.m_pArray, "a", srlztn::ArrayReader<uint16>(nSize));
}


void WriteModSequences(std::ostream& oStrm, const ModSequenceSet& seq)
//--------------------------------------------------------------------
{
	srlztn::SsbWrite ssb(oStrm);
	ssb.BeginWrite(FileIdSequences, MptVersion::num);
	const uint8 nSeqs = seq.GetNumSequences();
	const uint8 nCurrent = seq.GetCurrentSequenceIndex();
	ssb.WriteItem(nSeqs, "n");
	ssb.WriteItem(nCurrent, "c");
	for(uint8 i = 0; i < nSeqs; i++)
	{
		if (i == seq.GetCurrentSequenceIndex())
			ssb.WriteItem(seq, srlztn::ID::FromInt<uint8>(i), &WriteModSequence);
		else
			ssb.WriteItem(seq.m_Sequences[i], srlztn::ID::FromInt<uint8>(i), &WriteModSequence);
	}
	ssb.FinishWrite();
}


void ReadModSequences(std::istream& iStrm, ModSequenceSet& seq, const size_t)
//---------------------------------------------------------------------------
{
	srlztn::SsbRead ssb(iStrm);
	ssb.BeginRead(FileIdSequences, MptVersion::num);
	if ((ssb.GetStatus() & srlztn::SNT_FAILURE) != 0)
		return;
	uint8 nSeqs = 0;
	uint8 nCurrent = 0;
	ssb.ReadItem(nSeqs, "n");
	if (nSeqs == 0)
		return;
	LimitMax(nSeqs, MAX_SEQUENCES);
	ssb.ReadItem(nCurrent, "c");
	if (seq.GetNumSequences() < nSeqs)
		seq.m_Sequences.resize(nSeqs, ModSequence(seq.m_sndFile, seq.s_nCacheSize));

	for(uint8 i = 0; i < nSeqs; i++)
	{
		ssb.ReadItem(seq.m_Sequences[i], srlztn::ID::FromInt<uint8>(i), &ReadModSequence);
	}
	seq.m_nCurrentSeq = (nCurrent < seq.GetNumSequences()) ? nCurrent : 0;
	seq.CopyStorageToCache();
}


OPENMPT_NAMESPACE_END
