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

import QtQuick 2.2
import MuseScore 3.0

MuseScore {
    version: "3.5"
    description: qsTr("This plugin colors notes based on their degree relative to a root note")
    title: "Degree Colors"
    categoryCode: "degree-colors"
    thumbnailName: "color_notes.png"

    property variant colors: [
        "#B3D94C", // I Ionian (Bright Green)
        black,     // C#/Db
        "#4A90E2", // ii Dorian (Sky Blue)
        black,     // D#/Eb
        "#E74C3C", // iii Phrygian (Rich Red)
        "#F1C40F", // IV Lydian (Golden Yellow)
        black,     // F#/Gb
        "#00BFFF", // V Mixolydian (Vibrant Cyan)
        black,     // G#/Ab
        "#FF6F61", // vi Aeolian (Coral Pink)
        black,     // A#/Bb (4th degree in F major)
        "#FFA500"  // vii dim Locrian (Bright Orange)
    ]
    property string black: "#000000"
    property int rootNotePitch: 5 // Define the root note of the chord (F = 5 semitones from C)

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

    function getLowestNote(notes) {
        return notes.reduce((lowest, current) => (current.pitch < lowest.pitch ? current : lowest));
    }

    function getRootNoteOfChord(note) {
        const chord = note.chord;
        if (!chord || chord.notes.length === 0) return null;

        return chord.notes.reduce((root, current) => (current.pitch < root.pitch ? current : root));
    }

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

    onRun: {
        console.log("Running Degree Colors Plugin");

        applyToNotesInSelection(colorNote);
    }
}
