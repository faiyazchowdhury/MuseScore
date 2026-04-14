/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*!
 * \file
 * \brief Unit tests for pitch-class, scale-degree, and chord-root helpers in `notecoloringscheme.h`.
 */

#include <gtest/gtest.h>

#include "engraving/types/notecoloringscheme.h"

using namespace mu::engraving;

/*!
 * GoogleTest fixture for pure functions in @c notecoloringscheme.h.
 * No @c Score, layout, or engraving objects are constructed.
 */
class Engraving_NoteColoringTests : public ::testing::Test
{
};

//! @test `tonicPitchClassFromKey` matches the circle of fifths for sample key signatures.
TEST_F(Engraving_NoteColoringTests, tonicPitchClassFromKey)
{
    EXPECT_EQ(tonicPitchClassFromKey(0), 0);    // C major
    EXPECT_EQ(tonicPitchClassFromKey(1), 7);    // G major
    EXPECT_EQ(tonicPitchClassFromKey(-1), 5);   // F major
    EXPECT_EQ(tonicPitchClassFromKey(2), 2);    // D major
    EXPECT_EQ(tonicPitchClassFromKey(-7), 11); // 7 flats: Cb major (12-TET same as B)
}

//! @test Major-scale degree index from pitch class and tonic chroma.
TEST_F(Engraving_NoteColoringTests, pitchToDegreeIndex)
{
    const int tonicC = 0;
    EXPECT_EQ(pitchToDegreeIndex(0, tonicC), 0);   // C
    EXPECT_EQ(pitchToDegreeIndex(4, tonicC), 2);   // E
    EXPECT_EQ(pitchToDegreeIndex(11, tonicC), 6);   // B
    EXPECT_EQ(pitchToDegreeIndex(1, tonicC), -1);  // C# not in C major

    const int tonicG = 7;
    EXPECT_EQ(pitchToDegreeIndex(7, tonicG), 0);   // G
    EXPECT_EQ(pitchToDegreeIndex(11, tonicG), 2); // B (major third above G)
}

//! @test Chord-degree swatch indices for thirds and fifths above the estimated root.
TEST_F(Engraving_NoteColoringTests, chordDegreeColorIndex_thirdAndFifth)
{
    const int rootC = 0;
    const int tonicC = 0;
    EXPECT_EQ(chordDegreeColorIndex(4, rootC, tonicC), 7);  // major third
    EXPECT_EQ(chordDegreeColorIndex(3, rootC, tonicC), 7);  // minor third
    EXPECT_EQ(chordDegreeColorIndex(7, rootC, tonicC), 8);   // fifth
}

//! @test Diatonic chord-degree colors use 0-based major-scale steps from the key tonic.
TEST_F(Engraving_NoteColoringTests, chordDegreeColorIndex_diatonicDegree)
{
    const int rootC = 0;
    const int tonicC = 0;
    EXPECT_EQ(chordDegreeColorIndex(2, rootC, tonicC), 1);  // D = major-scale degree index 1 (0-based from tonic)
    EXPECT_EQ(chordDegreeColorIndex(9, rootC, tonicC), 5);  // A = major-scale degree index 5
}

//! @test Chromatic tones outside the key map to the dedicated accidental swatch bucket.
TEST_F(Engraving_NoteColoringTests, chordDegreeColorIndex_chromaticBucket)
{
    const int rootC = 0;
    const int tonicC = 0;
    EXPECT_EQ(chordDegreeColorIndex(1, rootC, tonicC), 9);  // C# outside C major
}

//! @test `findChordRoot` edge cases: empty input and a single pitch class.
TEST_F(Engraving_NoteColoringTests, findChordRoot_emptyAndSingle)
{
    EXPECT_EQ(findChordRoot({}), 0);
    EXPECT_EQ(findChordRoot({ 5 }), 5);
}

//! @test Major triad chromas favor the root under interval-weighted scoring.
TEST_F(Engraving_NoteColoringTests, findChordRoot_majorTriadPrefersRoot)
{
    // C major: chromas 0, 4, 7 — C should beat E and G as root under interval scoring
    EXPECT_EQ(findChordRoot({ 0, 4, 7 }), 0);
}

//! @test `analyzeChordRoot` returns 0 when there are no note events.
TEST_F(Engraving_NoteColoringTests, analyzeChordRoot_empty)
{
    EXPECT_EQ(analyzeChordRoot({}, 480), 0);
}

//! @test Full-window C major triad yields root chroma C (0).
TEST_F(Engraving_NoteColoringTests, analyzeChordRoot_simpleTriad)
{
    const int windowTicks = 1920;
    std::vector<ChordDegreeNoteInfo> infos = {
        { 60, 480, 0 }, // C4
        { 64, 480, 0 }, // E4
        { 67, 480, 0 }, // G4
    };
    EXPECT_EQ(analyzeChordRoot(infos, windowTicks), 0);
}

//! @test Bass (lowest pitch) bonus increases the chord-root candidate score.
TEST_F(Engraving_NoteColoringTests, scoreNoteForChord_lowestPitchBonus)
{
    int chromaCounts[12] = {};
    chromaCounts[0] = 1;
    chromaCounts[4] = 1;
    const int windowTicks = 1920;
    ChordDegreeNoteInfo low { 48, 100, 0 };   // C3 — lowest
    ChordDegreeNoteInfo high { 64, 100, 0 };  // E4
    int lowestPitch = 48;
    EXPECT_GT(scoreNoteForChord(low, windowTicks, lowestPitch, chromaCounts),
              scoreNoteForChord(high, windowTicks, lowestPitch, chromaCounts));
}

//! @test analyzeChordRoot in a 3/4 window still picks the correct root.
TEST_F(Engraving_NoteColoringTests, analyzeChordRoot_threeQuarterMeter)
{
    const int windowTicks = 1440; // 3 quarter notes at 480 ticks each
    std::vector<ChordDegreeNoteInfo> infos = {
        { 55, 480, 0 },    // G3 on beat 1 (strong)
        { 59, 480, 480 },  // B3 on beat 2
        { 62, 480, 960 },  // D4 on beat 3
    };
    // G major triad — root should be G (chroma 7)
    EXPECT_EQ(analyzeChordRoot(infos, windowTicks), 7);
}

//! @test Equal @c scoreNoteForChord values keep @c stable_sort order; @c analyzeChordRoot stays consistent.
TEST_F(Engraving_NoteColoringTests, scoreNoteForChord_tiebreakDeterministic)
{
    int chromaCounts[12] = {};
    chromaCounts[0] = 1;
    chromaCounts[7] = 1;
    const int windowTicks = 960;
    int lowestPitch = 40; // below C3 and G3 so neither note gets the lowest-pitch bonus
    ChordDegreeNoteInfo noteC { 48, 240, 0 };  // C3, beat 1
    ChordDegreeNoteInfo noteG { 55, 240, 0 };  // G3, beat 1, same duration/position
    int scoreC = scoreNoteForChord(noteC, windowTicks, lowestPitch, chromaCounts);
    int scoreG = scoreNoteForChord(noteG, windowTicks, lowestPitch, chromaCounts);
    EXPECT_EQ(scoreC, scoreG);

    std::vector<ChordDegreeNoteInfo> infosForward = { noteC, noteG };
    std::vector<ChordDegreeNoteInfo> infosBackward = { noteG, noteC };
    // C major dyad {0,7}: same resolved root regardless of input order; equal per-note scores stay stable.
    EXPECT_EQ(analyzeChordRoot(infosForward, windowTicks), 0);
    EXPECT_EQ(analyzeChordRoot(infosBackward, windowTicks), 0);
}
