/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
 *
 * Copyright (c) 2011 Peng Wu <alexepico@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "PYPBopomofoEditor.h"
#include "PYConfig.h"
#include "PYLibPinyin.h"
#include "PYPinyinProperties.h"
#include "PYSimpTradConverter.h"
#include "PYHalfFullConverter.h"


namespace PY {
#include "PYBopomofoKeyboard.h"
};

using namespace PY;

const static gchar * bopomofo_select_keys[] = {
    "1234567890",
    "asdfghjkl;",
    "1qaz2wsxed",
    "asdfzxcvgb",
    "1234qweras",
    "aoeu;qjkix",
    "aoeuhtnsid",
    "aoeuidhtns",
    "qweasdzxcr"
};

LibPinyinBopomofoEditor::LibPinyinBopomofoEditor
(PinyinProperties & props, Config & config)
    : LibPinyinPhoneticEditor (props, config),
      m_select_mode (FALSE)
{
    m_instance = LibPinyinBackEnd::instance ().allocChewingInstance ();
}

LibPinyinBopomofoEditor::~LibPinyinBopomofoEditor (void)
{
    LibPinyinBackEnd::instance ().freeChewingInstance (m_instance);
    m_instance = NULL;
}

void
LibPinyinBopomofoEditor::reset (void)
{
    m_select_mode = FALSE;
    LibPinyinPhoneticEditor::reset ();
}

gboolean
LibPinyinBopomofoEditor::insert (gint ch)
{
    /* is full */
    if (G_UNLIKELY (m_text.length () >= MAX_PINYIN_LEN))
        return TRUE;

    m_text.insert (m_cursor++, ch);
    updatePinyin ();
    update ();

    return TRUE;
}


gboolean
LibPinyinBopomofoEditor::processGuideKey (guint keyval, guint keycode,
                                          guint modifiers)
{
    if (!m_config.guideKey ())
        return FALSE;

    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return FALSE;

    if (G_LIKELY (m_select_mode))
        return FALSE;

    if (G_UNLIKELY (keyval == IBUS_space)) {
        m_select_mode = TRUE;
        update ();
        return TRUE;
    }

    return FALSE;
}

gboolean
LibPinyinBopomofoEditor::processAuxiliarySelectKey
(guint keyval, guint keycode, guint modifiers)
{
    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return FALSE;

    guint i;

    switch (keyval) {
    case IBUS_KP_0:
        i = 9;
        if (!m_config.auxiliarySelectKeyKP ())
            return FALSE;
        break;
    case IBUS_KP_1 ... IBUS_KP_9:
        i = keyval - IBUS_KP_1;
        if (!m_config.auxiliarySelectKeyKP ())
            return FALSE;
        break;
    case IBUS_F1 ... IBUS_F10:
        i = keyval - IBUS_F1;
        if (!m_config.auxiliarySelectKeyF ())
            return FALSE;
        break;
    default:
        return FALSE;
    }

    m_select_mode = TRUE;
    selectCandidateInPage (i);

    update ();
    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::processSelectKey (guint keyval, guint keycode,
                                           guint modifiers)
{
    if (G_UNLIKELY (!m_text))
        return FALSE;

    if (G_LIKELY (!m_select_mode && ((modifiers & IBUS_MOD1_MASK) == 0)))
        return FALSE;

    const gchar * pos = NULL;
    const gchar * keys = bopomofo_select_keys[m_config.selectKeys ()];
    for ( const gchar * p = keys; *p; ++p ) {
        if ( *p == keyval )
            pos = p;
    }

    if (pos == NULL)
        return FALSE;

    m_select_mode = TRUE;

    guint i = pos - keys;
    selectCandidateInPage (i);

    update ();
    return TRUE;
}

gboolean
LibPinyinBopomofoEditor::processBopomofo (guint keyval, guint keycode,
                                          guint modifiers)
{
    if (G_UNLIKELY (cmshm_filter (modifiers) != 0))
        return m_text ? TRUE : FALSE;

    if (keyvalToBopomofo (keyval) == BOPOMOFO_ZERO)
        return FALSE;

    m_select_mode = FALSE;

    return insert (keyval);
}

gboolean
LibPinyinBopomofoEditor::processKeyEvent (guint keyval, guint keycode,
                                          guint modifiers)
{
    modifiers &= (IBUS_SHIFT_MASK |
                  IBUS_CONTROL_MASK |
                  IBUS_MOD1_MASK |
                  IBUS_SUPER_MASK |
                  IBUS_HYPER_MASK |
                  IBUS_META_MASK |
                  IBUS_LOCK_MASK);

    if (G_UNLIKELY (processGuideKey (keyval, keycode, modifiers)))
        return TRUE;
    if (G_UNLIKELY (processSelectKey (keyval, keycode, modifiers)))
        return TRUE;
    if (G_UNLIKELY (processAuxiliarySelectKey (keyval, keycode,
                                               modifiers)))
        return TRUE;
    if (G_LIKELY (processBopomofo (keyval, keycode, modifiers)))
        return TRUE;

    switch (keyval) {
    case IBUS_space:
        m_select_mode = TRUE;
        return processSpace (keyval, keycode, modifiers);

    case IBUS_Up:        case IBUS_KP_Up:
    case IBUS_Down:      case IBUS_KP_Down:
    case IBUS_Page_Up:   case IBUS_KP_Page_Up:
    case IBUS_Page_Down: case IBUS_KP_Page_Down:
    case IBUS_Tab:
        m_select_mode = TRUE;
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);

    case IBUS_BackSpace:
    case IBUS_Delete:    case IBUS_KP_Delete:
    case IBUS_Left:      case IBUS_KP_Left:
    case IBUS_Right:     case IBUS_KP_Right:
    case IBUS_Home:      case IBUS_KP_Home:
    case IBUS_End:       case IBUS_KP_End:
        m_select_mode = FALSE;
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);

    default:
        return LibPinyinPhoneticEditor::processFunctionKey
            (keyval, keycode, modifiers);
    }
    return FALSE;
}

void
LibPinyinBopomofoEditor::updatePinyin (void)
{
    if (G_UNLIKELY (m_text.empty ())) {
        m_pinyin_len = 0;
        /* TODO: check whether to replace "" with NULL. */
        pinyin_parse_more_chewings (m_instance, NULL);
        return;
    }

    m_pinyin_len =
        pinyin_parse_more_chewings (m_instance, m_text.c_str ());
    pinyin_guess_sentence (m_instance);
}

gint
LibPinyinBopomofoEditor::keyvalToBopomofo(gint ch)
{
    const gint keyboard = m_config.bopomofoKeyboardMapping ();    
    gint len = G_N_ELEMENTS (bopomofo_keyboard[keyboard]);

    for ( gint i = 0; i < len; ++i ) {
        if ( bopomofo_keyboard[keyboard][i][0] == ch )
            return bopomofo_keyboard[keyboard][i][1];
    }

    return BOPOMOFO_ZERO;
}

void
LibPinyinBopomofoEditor::commit ()
{
    if (G_UNLIKELY (m_text.empty ()))
        return;

    m_buffer.clear ();

    /* sentence candidate */
    char *tmp = NULL;
    pinyin_get_sentence(m_instance, &tmp);
    if (m_props.modeSimp ()) {
        m_buffer << tmp;
    } else {
        SimpTradConverter::simpToTrad (tmp, m_buffer);
    }
    g_free (tmp);

    /* text after pinyin */
    const gchar *p = m_text.c_str() + m_pinyin_len;
    while (*p != '\0') {
        if (keyvalToBopomofo (*p)) {
            m_buffer << keyvalToBopomofo (*p);
        } else {
            if (G_UNLIKELY (m_props.modeFull ())) {
                m_buffer.appendUnichar (HalfFullConverter::toFull (*p++));
            } else {
                m_buffer << p;
            }
        }
    }

    pinyin_train(m_instance);
    LibPinyinBackEnd::instance ().modified();
    LibPinyinPhoneticEditor::commit ((const gchar *)m_buffer);
    reset();
}

void
LibPinyinBopomofoEditor::updatePreeditText ()
{
    /* preedit text = guessed sentence + un-parsed pinyin text */
    if (G_UNLIKELY (m_text.empty ())) {
        hidePreeditText ();
        return;
    }

    m_buffer.clear ();
    char *tmp = NULL;
    pinyin_get_sentence(m_instance, &tmp);
    if (tmp) {
        if (m_props.modeSimp ()) {
            m_buffer<<tmp;
        } else {
            SimpTradConverter::simpToTrad (tmp, m_buffer);
        }
        g_free (tmp);
        tmp = NULL;
    }

    /* append rest text */
    const gchar *p = m_text.c_str () + m_pinyin_len;
    m_buffer << p;

    StaticText preedit_text (m_buffer);
    /* underline */
    preedit_text.appendAttribute (IBUS_ATTR_TYPE_UNDERLINE, IBUS_ATTR_UNDERLINE_SINGLE, 0, -1);

    guint pinyin_cursor = getPinyinCursor ();
    Editor::updatePreeditText (preedit_text, pinyin_cursor, TRUE);
}

void
LibPinyinBopomofoEditor::updateAuxiliaryText (void)
{
    if (G_UNLIKELY (m_text.empty ())) {
        hideAuxiliaryText ();
        return;
    }

    m_buffer.clear ();

    // guint pinyin_cursor = getPinyinCursor ();
    PinyinKeyVector & pinyin_keys = m_instance->m_pinyin_keys;
    PinyinKeyPosVector & pinyin_poses = m_instance->m_pinyin_key_rests;
    for (guint i = 0; i < pinyin_keys->len; ++i) {
        PinyinKey *key = &g_array_index (pinyin_keys, PinyinKey, i);
        PinyinKeyPos *pos = &g_array_index (pinyin_poses, PinyinKeyPos, i);
        guint cursor = pos->m_raw_begin;

        if (G_UNLIKELY (cursor == m_cursor)) { /* at word boundary. */
            m_buffer << '|' << key->get_chewing_string ();
        } else if (G_LIKELY ( cursor < m_cursor &&
                              m_cursor < pos->m_raw_end )) { /* in word */
            /* raw text */
            String raw = m_text.substr (cursor,
                                        pos->m_raw_end - pos->m_raw_begin);
            guint offset = m_cursor - cursor;
            m_buffer << ' ';
            String before = raw.substr (0, offset);
            String after = raw.substr (offset);
            String::const_iterator iter;
            for ( iter = before.begin (); iter != before.end (); ++iter) {
                m_buffer << bopomofo_char[keyvalToBopomofo (*iter)];
            }
            m_buffer << '|';
            for ( iter = after.begin (); iter != after.end (); ++iter) {
                m_buffer << bopomofo_char[keyvalToBopomofo (*iter)];
            }
        } else { /* other words */
            m_buffer << ' ' << key->get_chewing_string ();
        }
    }

    if (m_cursor == m_pinyin_len)
        m_buffer << '|';

    /* append rest text */
    const gchar * p = m_text.c_str() + m_pinyin_len;
    m_buffer << p;

    StaticText aux_text (m_buffer);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

