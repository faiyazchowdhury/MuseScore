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
const colors = [ // "#rrggbb" with rr, gg, and bb being the hex values for red, green, and blue, respectively
    "#e21c48", // C
    "#f26622", // C#/Db
    "#f99d1c", // D
    "#ffcc33", // D#/Eb
    "#fff32b", // E
    "#bcd85f", // F
    "#62bc47", // F#/Gb
    "#009c95", // G
    "#0071bb", // G#/Ab
    "#5e50a1", // A
    "#8d5ba6", // A#/Bb
    "#cf3e96"  // B
];
const black = "#000000";

// Define the key of the song (e.g., C major = 0, C# major = 1, etc.)
const keyOfSong = 0; // Change this value to match the key of your song (0 = C, 1 = C#/Db, etc.)

function main() {
    api.log.info("TEST hello colornotes");

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

    let notes = [];
    for (var i in curScore.selection.elements) {
        if (curScore.selection.elements[i].pitch) {
            notes.push(curScore.selection.elements[i]);
        }
    }

    if (notes.length > 0) {
        const lowestNote = getLowestNote(notes);
        for (let note of notes) {
            func(note, lowestNote);
        }
        colorHorizontalLine(keyOfSong); // Color horizontal line based on the key of the song
    }

    curScore.endCmd();
    if (fullScore) {
        cmd("escape");
    }
}

// Function to get the lowest note from a list of notes
function getLowestNote(notes) {
    return notes.reduce((lowest, current) => (current.pitch < lowest.pitch ? current : lowest));
}

// Function to determine if a note is part of a chord and get its root note
function getRootNoteOfChord(note) {
    const chord = note.chord;
    if (!chord || chord.notes.length === 0) return null;
    
    return chord.notes.reduce((root, current) => (current.pitch < root.pitch ? current : root));
}

// Function to color a note based on its role (lowest note or root note)
function colorNote(note, lowestNote) {
    let isLowestNote = note === lowestNote;
    let rootNote = getRootNoteOfChord(note);

    if (isLowestNote) {
        note.color = "#00FFFF"; // Cyan for lowest note
    } else if (rootNote && note === rootNote) {
        note.color = "#FF00FF"; // Magenta for root note of a chord
    } else {
        note.color = colors[note.pitch % 12]; // Default coloring based on pitch
    }

    if (note.accidental) {
        note.accidental.color = note.color;
    }

    if (note.dots) {
        for (var i = 0; i < note.dots.length; i++) {
            if (note.dots[i]) {
                note.dots[i].color = note.color;
            }
        }
    }
}

// Function to color the horizontal line based on the key of the song
function colorHorizontalLine(keyOfSong) {
    const staffLines = curScore.selection.staffLines || curScore.staffLines;
    
    for (let line of staffLines) {
        line.color = colors[keyOfSong % 12]; // Use keyOfSong to determine line color
    }
}
