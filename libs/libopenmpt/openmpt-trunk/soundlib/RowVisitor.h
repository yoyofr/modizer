/*
 * RowVisitor.h
 * ------------
 * Purpose: Class for managing which rows of a song has already been visited. Useful for detecting backwards jumps, loops, etc.
 * Notes  : See implementation file.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <vector>
#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
class ModSequence;

class RowVisitor
{
protected:
	// Memory for every row in the module if it has been visited or not.
	std::vector<std::vector<bool>> m_visitedRows;
	// Memory of visited rows (including their order) to reset pattern loops.
	std::vector<ROWINDEX> m_visitOrder;

	const CSoundFile &m_sndFile;
	ORDERINDEX m_currentOrder;
	SEQUENCEINDEX m_sequence;

public:
	RowVisitor(const CSoundFile &sf, SEQUENCEINDEX sequence = SEQUENCEINDEX_INVALID);
	
	void MoveVisitedRowsFrom(RowVisitor &other);

	// Resize / Clear the row vector.
	// If reset is true, the vector is not only resized to the required dimensions, but also completely cleared (i.e. all visited rows are unset).
	void Initialize(bool reset);

	// Mark a row as visited.
	void Visit(ORDERINDEX ord, ROWINDEX row)
	{
		SetVisited(ord, row, true);
	}

	// Mark a row as not visited.
	void Unvisit(ORDERINDEX ord, ROWINDEX row)
	{
		SetVisited(ord, row, false);
	}

	// Returns whether a given row has been visited yet.
	// If autoSet is true, the queried row will automatically be marked as visited.
	// Use this parameter instead of consecutive IsRowVisited / SetRowVisited calls.
	bool IsVisited(ORDERINDEX ord, ROWINDEX row, bool autoSet);

	// Get the needed vector size for a given pattern.
	size_t GetVisitedRowsVectorSize(PATTERNINDEX pattern) const;

	// Find the first row that has not been played yet.
	// The order and row is stored in the order and row variables on success, on failure they contain invalid values.
	// If onlyUnplayedPatterns is true (default), only completely unplayed patterns are considered, otherwise a song can start anywhere.
	// Function returns true on success.
	bool GetFirstUnvisitedRow(ORDERINDEX &order, ROWINDEX &row, bool onlyUnplayedPatterns) const;

	// Set all rows of a previous pattern loop as unvisited.
	void ResetPatternLoop(ORDERINDEX ord, ROWINDEX startRow);

	// Returns the last visited row index of the current pattern, or ROWINDEX_INVALID if there is none.
	ROWINDEX GetLastVisitedRow() const;

protected:

	// (Un)sets a given row as visited.
	// order, row - which row should be (un)set
	// If visited is true, the row will be set as visited.
	void SetVisited(ORDERINDEX ord, ROWINDEX row, bool visited);

	// Add a row to the visited row memory for this pattern.
	void AddVisitedRow(ORDERINDEX ord, ROWINDEX row);

	const ModSequence &Order() const;
};

OPENMPT_NAMESPACE_END
