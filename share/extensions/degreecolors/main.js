/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited
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

const black = "#000000";


const colors = [ // "#rrggbb" with rr, gg, and bb being the hex values for red, green, and blue, respectively
    "#B3D94C", // I Ionian (Bright Green)
    black,      // C#/Db
    "#4A90E2", // ii Dorian (Sky Blue)
    black,      // D#/Eb
    "#E74C3C", // iii Phrygian (Rich Red)
    "#F1C40F", // IV Lydian (Golden Yellow)
    black,      // F#/Gb
    "#00BFFF", // V Mixolydian (Vibrant Cyan)
    black,      // G#/Ab
    "#FF6F61", // vi Aeolian (Coral Pink)
    black,      // A#/Bb (4th degree in F major)
    "#FFA500"  // vii dim Locrian (Bright Orange)
];

// Define the root note of the chord (F = 5 semitones from C)
const rootNotePitch = 5; 

function main() {
    api.log.info("hello degree colors");

    applyToNotesInSelection(colorNote);
}

// Apply the given function to all notes (elements with pitch) in selection
// or, if nothing is selected, in the entire score
function applyToNotesInSelection(func) {
    var fullScore = !curScore.selection.elements.length;
    if (fullScore) {
        cmd("select-all");
    }
    curScore.startCmd();

    let notesByBar = groupNotesByBar(curScore.selection.elements);

    for (let barNotes of notesByBar) {
        if (barNotes.length > 0) {
            const lowestNote = getLowestNote(barNotes);
            const rootNote = getRootNoteOfChord(barNotes[0]); // Assuming chord root is consistent within a bar

            for (let note of barNotes) {
                func(note, lowestNote, rootNote);
            }
        }
    }

    curScore.endCmd();
    if (fullScore) {
        cmd("escape");
    }
}

// Group notes by their bar position
function groupNotesByBar(notes) {
    let bars = {};
    
    for (let note of notes) {
        if (!note.pitch) continue;

        const barNumber = note.segment.tick / curScore.ticksPerMeasure;
        if (!bars[barNumber]) bars[barNumber] = [];
        bars[barNumber].push(note);
    }

    return Object.values(bars);
}

// Get the lowest note from a list of notes
function getLowestNote(notes) {
    return notes.reduce((lowest, current) => (current.pitch < lowest.pitch ? current : lowest));
}

// Determine the root note of a chord
function getRootNoteOfChord(note) {
    const chord = note.chord;
    if (!chord || chord.notes.length === 0) return null;

    return chord.notes.reduce((root, current) => (current.pitch < root.pitch ? current : root));
}

// Color a note based on its role: root or lowest non-root note
function colorNote(note, lowestNote, rootNote) {
    if (!note || !note.pitch) return;

    let color;
    
    if (note === rootNote) {
        color = colors[rootNotePitch % 12]; // Color based on root pitch
    } else if (note === lowestNote && lowestNote !== rootNote) {
        color = colors[(lowestNote.pitch - rootNotePitch + 12) % 12]; // Degree relative to root
    } else {
        color = black; // Default to black for uncolored notes
    }

    note.color = color;

    if (note.accidental) {
        note.accidental.color = color;
    }

    if (note.dots) {
        for (var i = 0; i < note.dots.length; i++) {
            if (note.dots[i]) {
                note.dots[i].color = color;
            }
        }
    }
}
