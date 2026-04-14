/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
 */

/*!
 * \file
 * \brief Note coloring schemes and chord-degree harmonic analysis for engraving and Edit Style.
 *
 * Defines @c NoteColoringScheme, @c ChordDegreeNoteInfo, @c analyzeChordRoot(), @c chordDegreeColorIndex(),
 * and related pitch-class helpers used by @c Note and selection coloring.
 */
#pragma once

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace mu::engraving {
//! Score bonus when the note onset aligns with strong beats in @c scoreNoteForChord().
inline constexpr int CHORD_ROOT_SCORE_ON_BEAT_BONUS = 100;
//! Score bonus when the note is the lowest pitch among candidates (bass preference).
inline constexpr int CHORD_ROOT_SCORE_LOWEST_PITCH_BONUS = 150;
//! Weight per occurrence of the note's chroma in the window (salience).
inline constexpr int CHORD_ROOT_SCORE_CHROMA_COUNT_WEIGHT = 20;
//! Divisor mapping window length to a quarter-note tick grid for beat emphasis.
inline constexpr int CHORD_ROOT_SCORE_QUARTER_DIVISOR = 4;
//! Interval weight in @c findChordRoot() for perfect fifth above a candidate root.
inline constexpr int CHORD_ROOT_SCORE_INTERVAL_FIFTH = 3;
//! Interval weight in @c findChordRoot() for major/minor third above a candidate root.
inline constexpr int CHORD_ROOT_SCORE_INTERVAL_THIRD = 2;
//! Interval weight in @c findChordRoot() for seventh above a candidate root.
inline constexpr int CHORD_ROOT_SCORE_INTERVAL_SEVENTH = 1;

/*!
 * Style-driven scheme for mapping notes to palette swatches (@c Sid::noteColor0 ... @c Sid::noteColor11).
 */
enum class NoteColoringScheme : int {
    OneColor = 0, //!< Single user color for all notes.
    AbsolutePitchSimple = 1,      //!< Step index from TPC (letter name / diatonic step).
    AbsolutePitchChromatic = 2,   //!< MIDI chroma from effective pitch.
    MoveableDoSimple = 3,         //!< Scale degree from TPC and local key.
    MoveableDoChromatic = 4,      //!< Chromatic scale degree relative to key tonic.
    ChordDegrees = 5              //!< Harmonic analysis: root, third, fifth, diatonic degrees, chromatic bucket.
};

/*! Semitone offsets of the major scale from the tonic (pitch classes). */
inline constexpr int MAJOR_SCALE_INTERVALS[] = { 0, 2, 4, 5, 7, 9, 11 };

/*!
 * Pitch class (0-11) of the major tonic for a key signature given as circle-of-fifths index.
 * @param keyFifths @c Key enum value (fifths from C).
 * @return Tonic pitch class 0-11.
 */
inline int tonicPitchClassFromKey(int keyFifths)
{
    int pc = (keyFifths * 7) % 12;
    return pc < 0 ? pc + 12 : pc;
}

/*!
 * Diatonic degree index 0-6 for @p pitchClass in a major scale built on @p tonicPC, or -1 if not in scale.
 * @param pitchClass Note chroma 0-11.
 * @param tonicPC Major-key tonic chroma 0-11.
 * @return Major scale degree index, or -1.
 */
inline int pitchToDegreeIndex(int pitchClass, int tonicPC)
{
    int semitones = (pitchClass - tonicPC + 12) % 12;
    for (int i = 0; i < 7; ++i) {
        if (MAJOR_SCALE_INTERVALS[i] == semitones) {
            return i;
        }
    }
    return -1;
}

/*!
 * One scored note inside a time window for @c analyzeChordRoot().
 * @p pitch and @p tickInWindow must use the same written/concert basis as @c Note coloring.
 */
struct ChordDegreeNoteInfo {
    int pitch;           //!< MIDI pitch in the same written/concert basis as @c Note automatic coloring.
    int durationTicks;   //!< Chord duration in ticks (longer harmony weights more).
    int tickInWindow;    //!< Offset from analysis window start in ticks (on-beat bonus in @c scoreNoteForChord()).
};

/*!
 * Heuristic score for how strongly @p info suggests belonging to the chord root set.
 * @param info Note event in the analysis window.
 * @param windowTicks Length of the window in ticks (for beat grid).
 * @param lowestPitch Minimum pitch among candidates (bass preference).
 * @param chromaCounts How often each chroma 0-11 appears (salience).
 * @return Larger means more likely root-related.
 * @note Beat strength uses @c windowTicks / @c CHORD_ROOT_SCORE_QUARTER_DIVISOR as a quarter-note grid (approximation, not full meter).
 */
inline int scoreNoteForChord(const ChordDegreeNoteInfo& info, int windowTicks,
                             int lowestPitch, const int chromaCounts[12])
{
    int score = info.durationTicks;

    int quarterTicks = windowTicks / CHORD_ROOT_SCORE_QUARTER_DIVISOR;
    if (quarterTicks > 0) {
        int beat = info.tickInWindow / quarterTicks;
        if (beat == 0 || beat == 2) {
            score += CHORD_ROOT_SCORE_ON_BEAT_BONUS;
        }
    }

    if (info.pitch == lowestPitch) {
        score += CHORD_ROOT_SCORE_LOWEST_PITCH_BONUS;
    }

    int chroma = info.pitch % 12;
    if (chroma < 0) {
        chroma += 12;
    }
    score += chromaCounts[chroma] * CHORD_ROOT_SCORE_CHROMA_COUNT_WEIGHT;

    return score;
}

/*!
 * Picks a root pitch class from candidate pitch classes using interval-weighted scoring.
 * @param pitchClasses Distinct chroma values (typically top-weighted from @c analyzeChordRoot()).
 * @return Chosen root pitch class, 0-11.
 * @note When two candidates share the same interval score, the earlier entry in @p pitchClasses wins.
 */
inline int findChordRoot(const std::vector<int>& pitchClasses)
{
    if (pitchClasses.empty()) {
        return 0;
    }
    if (pitchClasses.size() == 1) {
        return pitchClasses[0];
    }

    int bestRoot = pitchClasses[0];
    int highestScore = -1;

    for (size_t i = 0; i < pitchClasses.size(); ++i) {
        int potentialRoot = pitchClasses[i];
        int score = 0;

        for (size_t j = 0; j < pitchClasses.size(); ++j) {
            if (i == j) {
                continue;
            }
            int interval = (pitchClasses[j] - potentialRoot + 12) % 12;
            if (interval == 7) {
                score += CHORD_ROOT_SCORE_INTERVAL_FIFTH;
            } else if (interval == 4 || interval == 3) {
                score += CHORD_ROOT_SCORE_INTERVAL_THIRD;
            } else if (interval == 10 || interval == 11) {
                score += CHORD_ROOT_SCORE_INTERVAL_SEVENTH;
            }
        }

        if (score > highestScore) {
            highestScore = score;
            bestRoot = potentialRoot;
        }
    }

    return bestRoot;
}

/*!
 * Maps note chroma, measure root, and key tonic to a swatch index for @c NoteColoringScheme::ChordDegrees.
 * Thirds and fifths map to dedicated grey swatches; diatonic degrees use 0-6; chromatic/outside uses accidental slot.
 * @param pitchClass Note chroma 0-11.
 * @param rootChroma Estimated chord root chroma 0-11.
 * @param tonicPC Key tonic chroma 0-11 in the same pitch basis.
 * @return Swatch index (typically 0-9; thirds/fifths use dedicated slots).
 */
inline int chordDegreeColorIndex(int pitchClass, int rootChroma, int tonicPC)
{
    int semitonesFromRoot = (pitchClass - rootChroma + 12) % 12;
    if (semitonesFromRoot == 3 || semitonesFromRoot == 4) {
        return 7;
    } else if (semitonesFromRoot == 7) {
        return 8;
    } else {
        int deg = pitchToDegreeIndex(pitchClass, tonicPC);
        return (deg >= 0) ? deg : 9;
    }
}

/*!
 * Combines weighted note evidence in a time window to estimate a single chord-root chroma.
 * @param noteInfos Notes with pitch, duration, and position; pitches must share one coloring basis.
 * @param windowTicks Window length in ticks.
 * @return Root pitch class 0-11, or 0 if @p noteInfos is empty.
 * @note Chromas are ranked with @c std::stable_sort so equal scores keep the input order before taking the top four.
 */
inline int analyzeChordRoot(const std::vector<ChordDegreeNoteInfo>& noteInfos, int windowTicks)
{
    if (noteInfos.empty()) {
        return 0;
    }

    int lowestPitch = std::numeric_limits<int>::max();
    int chromaCounts[12] = {};
    for (const auto& info : noteInfos) {
        int chroma = info.pitch % 12;
        if (chroma < 0) {
            chroma += 12;
        }
        chromaCounts[chroma]++;
        if (info.pitch < lowestPitch) {
            lowestPitch = info.pitch;
        }
    }

    std::vector<std::pair<int, int> > scored;
    scored.reserve(noteInfos.size());
    for (const auto& info : noteInfos) {
        int chroma = info.pitch % 12;
        if (chroma < 0) {
            chroma += 12;
        }
        scored.push_back({ scoreNoteForChord(info, windowTicks, lowestPitch, chromaCounts), chroma });
    }
    std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    std::vector<int> topPitchClasses;
    bool seen[12] = {};
    for (const auto& s : scored) {
        if (!seen[s.second]) {
            seen[s.second] = true;
            topPitchClasses.push_back(s.second);
            if (topPitchClasses.size() >= 4) {
                break;
            }
        }
    }

    return findChordRoot(topPitchClasses);
}
}
