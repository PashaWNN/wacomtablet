/*
 * This file is part of the KDE wacomtablet project. For copyright
 * information and license terms see the AUTHORS and COPYING files
 * in the top-level directory of this distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// this include always needs to be the first one!
#include "debug.h"

#include "buttonshortcut.h"

#include <QtCore/QRegExp>
#include <QKeySequence>

#include <KDE/KLocalizedString>
#include <KDE/KGlobalAccel>


using namespace Wacom;

/*
 * When mapping multiple keys to the same name, the last entry
 * will be the default one. This is because QMap.find() returns
 * the most recently added value as default.
 *
 * Example:
 *
 * super_l = meta
 * super   = meta  <-- default entry when searching for "meta"
 *
 */
const char* ButtonShortcut::CONVERT_KEY_MAP_DATA[][2] = {
    {"alt_l",        "alt"},
    {"alt_r",        "alt"},
    {"alt",          "alt"},  // "alt" default
    {"ampersand",    "&"},
    {"apostrophe",   "'"},
    {"asciicircum",  "^"},
    {"asciitilde",   "~"},
    {"asterisk",     "*"},
    {"at",           "@"},
    {"backslash",    "\\"},
    {"bar",          "|"},
    {"braceleft",    "{"},
    {"braceright",   "}"},
    {"bracketleft",  "["},
    {"bracketright", "]"},
    {"colon",        ":"},
    {"comma",        ","},
    {"ctrl_l",       "ctrl"},
    {"ctrl_r",       "ctrl"},
    {"ctrl",         "ctrl"}, // "ctrl" default
    {"dollar",       "$"},
    {"equal",        "="},
    {"exclam",       "!"},
    {"greater",      ">"},
    {"less",         "<"},
    {"pgdn",         "pgdown"},
    {"numbersign",   "#"},
    {"period",       "."},
    {"plus",         "+"},
    {"minus",        "-"},
    {"parenleft",    "("},
    {"parenright",   ")"},
    {"percent",      "%"},
    {"question",     "?"},
    {"quotedbl",     "\""},
    {"quoteleft",    "`"},
    {"semicolon",    ";"},
    {"slash",        "/"},
    {"super_l",      "meta"},
    {"super_r",      "meta"},
    {"super",        "meta"}, // "super" default
    {"underscore",   "_"},
    {NULL, NULL}
};

namespace Wacom {
    class ButtonShortcutPrivate {
        public:
            ButtonShortcutPrivate() {
                type   = ButtonShortcut::NONE;
                button = 0;
            }

            ButtonShortcut::ShortcutType type;     //!< The shortcut type.
            QString                      sequence; //!< The key or modifier sequence.
            int                          button;   //!< The button number.
    };
}


ButtonShortcut::ButtonShortcut() : d_ptr(new ButtonShortcutPrivate)
{
}


ButtonShortcut::ButtonShortcut(const ButtonShortcut& that) : d_ptr(new ButtonShortcutPrivate)
{
    operator=(that);
}


ButtonShortcut::ButtonShortcut(const QString& shortcut) : d_ptr(new ButtonShortcutPrivate)
{
    set(shortcut);
}


ButtonShortcut::ButtonShortcut(int buttonNumber) : d_ptr(new ButtonShortcutPrivate)
{
    setButton(buttonNumber);
}


ButtonShortcut::~ButtonShortcut()
{
    delete this->d_ptr;
}


ButtonShortcut& ButtonShortcut::operator=(const ButtonShortcut& that)
{
    Q_D ( ButtonShortcut );

    *d = *(that.d_ptr);
    return *this;
}


ButtonShortcut& ButtonShortcut::operator=(const QString& shortcut)
{
    set(shortcut);
    return *this;
}


bool ButtonShortcut::operator==(const ButtonShortcut& that) const
{
    Q_D ( const ButtonShortcut );

    if ( d->type != that.d_ptr->type ) {
        return false;
    }

    if ( d->button != that.d_ptr->button ) {
        return false;
    }

    if ( d->sequence.compare(that.d_ptr->sequence, Qt::CaseInsensitive) != 0 ) {
        return false;
    }

    return true;
}


bool ButtonShortcut::operator!=(const ButtonShortcut& that) const
{
    return (!operator==(that));
}


void ButtonShortcut::clear()
{
    Q_D ( ButtonShortcut );

    d->type   = ButtonShortcut::NONE;
    d->button = 0;
    d->sequence.clear();
}


int ButtonShortcut::getButton() const
{
    Q_D (const ButtonShortcut);
    return d->button;
}


ButtonShortcut::ShortcutType ButtonShortcut::getType() const
{
    Q_D( const ButtonShortcut );
    return d->type;
}


bool ButtonShortcut::isButton() const
{
    return (getType() == BUTTON);
}


bool ButtonShortcut::isKeystroke() const
{
    return (getType() == KEYSTROKE);
}


bool ButtonShortcut::isModifier() const
{
    return (getType() == MODIFIER);
}


bool ButtonShortcut::isSet() const
{
    return (getType() != NONE);
}


bool ButtonShortcut::setButton(int buttonNumber)
{
    Q_D ( ButtonShortcut );

    clear();

    if (buttonNumber > 0 && buttonNumber <= 32) {
        d->type   = ButtonShortcut::BUTTON;
        d->button = buttonNumber;
        return true;
    }

    return false;
}


bool ButtonShortcut::set(const QString& sequence)
{
    clear();

    QString seq = sequence.trimmed();

    if (seq.isEmpty()) {
        return true;
    }

    QRegExp modifierRx (QLatin1String("^(?:key )?(?:\\s*\\+?(?:alt|ctrl|meta|shift|super))+$"), Qt::CaseInsensitive);
    QRegExp buttonRx (QLatin1String ("^(?:button\\s+)?\\+?\\d+$"), Qt::CaseInsensitive);

    if (seq.contains(buttonRx)) {
        // this is a button
        return setButtonSequence(seq);

    } else if (seq.contains(modifierRx)) {
        // this is a modifier sequence
        return setModifierSequence(seq);
    }

    // this is probably a key sequence
    return setKeySequence(seq);
}


const QString ButtonShortcut::toDisplayString() const
{
    Q_D (const ButtonShortcut);

    QList< KGlobalShortcutInfo > globalShortcutList;
    QString                      displayString;
    int                          buttonNr = getButton();

    switch (d->type) {
        case BUTTON:
            if (buttonNr == 1) {
                displayString = i18nc("Tablet button triggers a left mouse button click.", "Left Mouse Button Click");
            } else if (buttonNr == 2) {
                displayString = i18nc("Tablet button triggers a middle mouse button click.", "Middle Mouse Button Click");
            } else if (buttonNr == 3) {
                displayString = i18nc("Tablet button triggers a right mouse button click.", "Right Mouse Button Click");
            } else if (buttonNr == 4) {
                displayString = i18nc("Tablet button triggers mouse wheel up.", "Mouse Wheel Up");
            } else if (buttonNr == 5) {
                displayString = i18nc("Tablet button triggers mouse wheel down.", "Mouse Wheel Down");
            } else {
                displayString = i18nc("Tablet button triggers a click of mouse button with number #", "Mouse Button %1 Click", buttonNr);
            }
            break;

        case MODIFIER:
            displayString = d->sequence;
            convertKeySequenceToQKeySequenceFormat(displayString);
            break;

        case KEYSTROKE:
            displayString = d->sequence;
            convertKeySequenceToQKeySequenceFormat(displayString);

            // check if a global shortcut is assigned to this sequence
            globalShortcutList = KGlobalAccel::getGlobalShortcutsByKey(QKeySequence(displayString));

            if(!globalShortcutList.isEmpty()) {
                displayString = globalShortcutList.at(0).uniqueName();
            }
            break;

        case NONE:
            break;

        default:
            kDebug() << QString::fromLatin1("INTERNAL ERROR: Invalid type '%1' detected in ButtonShortcut!").arg(d->type);
    }

    return displayString;
}


const QString ButtonShortcut::toQKeySequenceString() const
{
    Q_D (const ButtonShortcut);

    QString keySequence;

    if (d->type == KEYSTROKE) {
        keySequence = d->sequence;
        convertKeySequenceToQKeySequenceFormat(keySequence);
    }

    return keySequence;
}


const QString ButtonShortcut::toString() const
{
    Q_D (const ButtonShortcut);

    QString shortcutString = QLatin1String("0");

    switch (d->type) {

        case BUTTON:
            shortcutString = QString::number(d->button);
            break;

        case MODIFIER:
        case KEYSTROKE:
            shortcutString = QString::fromLatin1("key %2").arg(d->sequence);
            break;

        case NONE:
            break;

        default:
            kDebug() << QString::fromLatin1("INTERNAL ERROR: Invalid type '%1' detected in ButtonShortcut!").arg(d->type);
    }

    return shortcutString.toLower();
}


bool ButtonShortcut::convertKey(QString& key, bool fromStorage) const
{
    QMap<QString,QString>::ConstIterator iter;
    QMap<QString,QString>::ConstIterator iterEnd;

    if (fromStorage) {
        iter    = getConvertFromStorageMap().find(key.toLower());
        iterEnd = getConvertFromStorageMap().constEnd();
    } else {
        iter    = getConvertToStorageMap().find(key.toLower());
        iterEnd = getConvertToStorageMap().constEnd();
    }

    if (iter == iterEnd) {
        return false;
    }

    key = iter.value();

    return true;
}


void ButtonShortcut::convertToNormalizedKeySequence(QString& sequence, bool fromStorage) const
{
    normalizeKeySequence(sequence);

    QStringList keyList    = sequence.split (QRegExp (QLatin1String ("\\s+")), QString::SkipEmptyParts);
    bool        isFirstKey = true;

    sequence.clear();

    for (QStringList::iterator iter = keyList.begin() ; iter != keyList.end() ; ++iter) {

        convertKey (*iter, fromStorage);
        prettifyKey (*iter);

        if (isFirstKey) {
            sequence.append(*iter);
            isFirstKey = false;
        } else {
            sequence.append(QString::fromLatin1(" %1").arg(*iter));
        }
    }
}


void ButtonShortcut::convertKeySequenceToStorageFormat(QString& sequence) const
{
    convertToNormalizedKeySequence(sequence, false);
}


void ButtonShortcut::convertKeySequenceToQKeySequenceFormat(QString& sequence) const
{
    convertToNormalizedKeySequence(sequence, true);
    sequence.replace(QLatin1String(" "), QLatin1String("+"));
}



const QMap< QString, QString >& ButtonShortcut::getConvertFromStorageMap()
{
    static QMap<QString, QString> map = initConversionMap(true);
    return map;
}


const QMap< QString, QString >& ButtonShortcut::getConvertToStorageMap()
{
    static QMap<QString, QString> map = initConversionMap(false);
    return map;
}


QMap< QString, QString > ButtonShortcut::initConversionMap(bool fromStorageMap)
{
    QMap<QString, QString> map;

    for (int i = 0 ; ; ++i) {
        if (CONVERT_KEY_MAP_DATA[i][0] == NULL || CONVERT_KEY_MAP_DATA[i][1] == NULL) {
            return map;
        }

        if (fromStorageMap) {
            map.insert(QLatin1String(CONVERT_KEY_MAP_DATA[i][0]), QLatin1String(CONVERT_KEY_MAP_DATA[i][1]));
        } else {
            map.insert(QLatin1String(CONVERT_KEY_MAP_DATA[i][1]), QLatin1String(CONVERT_KEY_MAP_DATA[i][0]));
        }
    }

    return map;
}


void ButtonShortcut::normalizeKeySequence(QString& sequence) const
{
    // When setting a shortcut like "ctrl+x", xsetwacom will convert it to "key +ctrl +x -x"
    // therefore we just truncate the string on the first "-key" we find.
    QRegExp minusKeyRx (QLatin1String ("(^|\\s)-\\S"));
    int     pos = 0;

    if ((pos = minusKeyRx.indexIn(sequence, 0)) != -1) {
        sequence = sequence.left(pos);
    }

    // cleanup leading "key " identifier from xsetwacom sequences
    sequence.remove(QRegExp (QLatin1String ("^\\s*key\\s+"), Qt::CaseInsensitive));

    // Remove all '+' prefixes from keys.
    // This will convert shortcuts like "+ctrl +alt" to "ctrl alt", but not
    // shortcuts like "ctrl +" which is required to keep compatibility to older
    // (buggy) configuration files.
    sequence.replace(QRegExp (QLatin1String ("(^|\\s)\\+(\\S)")), QLatin1String ("\\1\\2"));

    // Cleanup plus signs between keys.
    // This will convert shortcuts like "ctrl+alt+shift" or "Ctrl++" to "ctrl alt shift" or "Ctrl +".
    sequence.replace (QRegExp (QLatin1String ("(\\S)\\+(\\S)")), QLatin1String ("\\1 \\2"));

    // replace multiple whitespaces with one
    sequence.replace (QRegExp (QLatin1String ("\\s{2,}")), QLatin1String (" "));

    // trim the string
    sequence = sequence.trimmed();
}


void ButtonShortcut::prettifyKey(QString& key) const
{
    if (!key.isEmpty()) {
        key = key.toLower();
        key[0] = key.at(0).toUpper();
    }
}


bool ButtonShortcut::setButtonSequence(const QString& buttonSequence)
{
    QString buttonNumber = buttonSequence;
    buttonNumber.remove(QRegExp (QLatin1String ("^\\s*button\\s+"), Qt::CaseInsensitive));

    bool ok     = false;
    int  button = buttonNumber.toInt(&ok);

    if (!ok) {
        return false;
    }

    return setButton(button);
}


bool ButtonShortcut::setKeySequence(QString sequence)
{
    Q_D (ButtonShortcut);

    clear();

    // Check if this keysequence is valid by converting it to a QKeySequence and then back
    // again. If both sequences are equal the sequence should be valid. This is not very
    // effective but it leaves us with something KDE can handle and it makes sure a key
    // is always converted to its default key name if there are multiple mappings.
    // TODO improve this
    QString convertedSequence = sequence;
    convertKeySequenceToQKeySequenceFormat(convertedSequence);

    QKeySequence qkeySequence(convertedSequence);
    convertedSequence = qkeySequence.toString();

    convertKeySequenceToStorageFormat(convertedSequence);
    convertKeySequenceToStorageFormat(sequence);

    // we do not allow invalid sequences to be set
    // the sequences have to be equal and only one sequence is allowed
    if (sequence.compare(convertedSequence, Qt::CaseInsensitive) != 0 || qkeySequence.count() != 1) {
        return false;
    }

    d->type     = ButtonShortcut::KEYSTROKE;
    d->sequence = sequence;

    return true;
}


bool ButtonShortcut::setModifierSequence(QString sequence)
{
    Q_D (ButtonShortcut);

    clear();
    convertKeySequenceToStorageFormat(sequence);

    d->type     = ButtonShortcut::MODIFIER;
    d->sequence = sequence;

    return true;
}
