#include "actions.h"

#include "common.h"

#include <GLFW/glfw3.h>

#include "editor.h"
#include "history.h"
#include "os.h"
#include "platform_types.h"
#include "string_builder.h"
#include "text_buffer.h"
#include "util.h"

bool action_change_working_dir(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    View *view = create_buffer_view_prompt(
        "Change working dir:",
        prompt_create_context_change_working_dir(),
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 500, 100},
        state);
    Text_Line current_path_line = text_line_make_f("%s", state->working_dir);
    text_buffer_insert_line(&view->bv.buffer->text_buffer, current_path_line, 1);
    view->bv.cursor.pos = cursor_pos_to_end_of_line(view->bv.buffer->text_buffer, (Cursor_Pos){1, 0});
    return true;
}

bool action_live_scene_toggle_capture_input(Editor_State *state)
{
    if (!state->input_capture_live_scene_view)
    {
        if (state->active_view && state->active_view->kind == VIEW_KIND_LIVE_SCENE)
        {
            state->input_capture_live_scene_view = &state->active_view->lsv;
            Live_Scene *ls = state->input_capture_live_scene_view->live_scene;
            ls->dylib.on_platform_event(ls->state, &(Platform_Event){
                .kind = PLATFORM_EVENT_INPUT_CAPTURED,
                .input_captured.captured = true
            });
            return true;
        }
    }
    else
    {
        Live_Scene *ls = state->input_capture_live_scene_view->live_scene;
        ls->dylib.on_platform_event(ls->state, &(Platform_Event){
            .kind = PLATFORM_EVENT_INPUT_CAPTURED,
            .input_captured.captured = false
        });
        state->input_capture_live_scene_view = NULL;
        return true;
    }
    return false;
}

bool action_debug_break(Editor_State *state)
{
    state->should_break = true;
    return true;
}

bool action_destroy_active_view(Editor_State *state)
{
    // TODO: Should this be view level action?
    //       then it doesn't have to target "active" specifically
    //       and do this weird "will_propagate_to_view" logic.
    //       Although, it does make sense, to "kill view" from outside the view
    if (state->active_view != NULL)
    {
        view_destroy(state->active_view, state);
    }
    return true;
}

bool action_open_test_file1(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_buffer_view_open_file(
        FILE_PATH1,
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 500, 500},
        state);
    return true;
}

bool action_open_test_image(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_image_view(
        IMAGE_PATH,
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 500, 500},
        state);
    return true;
}

bool action_open_test_live_scene(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_live_scene_view(
        LIVE_CUBE_PATH,
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 500, 500},
        state);
    return true;
}

bool action_prompt_open_file(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_buffer_view_prompt(
        "Open file:",
        prompt_create_context_open_file(),
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 400, 100},
        state);
    return true;
}

bool action_prompt_new_file(Editor_State *state)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_buffer_view_generic(
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 500, 500},
        state);
    return true;
}

const char *_action_save_workspace_get_view_kind_str(View_Kind kind)
{
    switch (kind)
    {
        case VIEW_KIND_BUFFER:
        {
            return "BUFFER";
        } break;

        case VIEW_KIND_IMAGE:
        {
            return "IMAGE";
        } break;

        case VIEW_KIND_LIVE_SCENE:
        {
            return "LIVE_SCENE";
        } break;
    }
}

void _action_save_workspace_save_text_buffer_to_temp_loc(Text_Buffer tb, String_Builder *sb)
{
    time_t now = time(NULL);
    int r = rand() % 100000;
    char *temp_path = strf(E2_TEMP_FILES "/%ld_%d.tmp", now, r);
    text_buffer_write_to_file(tb, temp_path);
    string_builder_append_f(sb, "  TEMP_PATH='%s'\n", temp_path);
    free(temp_path);
}

bool action_save_workspace(Editor_State *state)
{
    clear_dir(E2_TEMP_FILES);

    String_Builder sb = {0};

    string_builder_append_f(&sb, "WORK_DIR='%s'\n", state->working_dir);
    string_builder_append_f(&sb, "CANVAS_POS=(%.3f,%.3f)\n", state->canvas_viewport.rect.x, state->canvas_viewport.rect.y);
    string_builder_append_f(&sb, "BUF_SEED=%d\n", state->buffer_seed);

    // Save views in reverse order, so it's easier to load (last loaded view is in the front of the queue)
    for (int i = state->view_count - 1; i >= 0; i--)
    {
        View *view = state->views[i];
        string_builder_append_f(&sb, "VIEW={\n");
        {
            string_builder_append_f(&sb, "  KIND=%s\n", _action_save_workspace_get_view_kind_str(view->kind));
            string_builder_append_f(&sb, "  RECT=(%.3f,%.3f,%.3f,%.3f)\n", view->outer_rect.x, view->outer_rect.y, view->outer_rect.w, view->outer_rect.h);

            switch (view->kind)
            {
                case VIEW_KIND_BUFFER:
                {
                    Buffer_View *bv = &view->bv;
                    string_builder_append_f(&sb, "  BUF_ID=%d\n", bv->buffer->id);
                    string_builder_append_f(&sb, "  BUF_VIEWPORT=(%.3f,%.3f,%.3f)\n", bv->viewport.rect.x, bv->viewport.rect.y, bv->viewport.zoom);
                    string_builder_append_f(&sb, "  CURSOR=(%d,%d)\n", bv->cursor.pos.line, bv->cursor.pos.col);
                    if (bv->mark.active)
                    {
                        string_builder_append_f(&sb, "  MARK=(%d,%d)\n", bv->mark.pos.line, bv->mark.pos.col);
                    }

                    Buffer *b = bv->buffer;
                    if (b->prompt_context.kind == PROMPT_NONE) // Don't save prompt views
                    {
                        _action_save_workspace_save_text_buffer_to_temp_loc(b->text_buffer, &sb);
                        if (b->file_path)
                        {
                            string_builder_append_f(&sb, "  FILE_PATH='%s'\n", b->file_path);
                        }
                    }
                } break;

                case VIEW_KIND_LIVE_SCENE:
                {
                    Live_Scene *ls = view->lsv.live_scene;
                    string_builder_append_f(&sb, "  DL_PATH='%s'\n", ls->dylib.original_path);
                    // TODO: Save live scene state?!!
                } break;

                case VIEW_KIND_IMAGE:
                {
                    log_warning("Image view saving not implemented");
                } break;
            }
        }
        string_builder_append_f(&sb, "}\n");
    }

    char *content = string_builder_compile_and_destroy(&sb);
    file_write(E2_WORKSPACE, content);
    free(content);

    return true;
}

bool action_buffer_view_move_cursor(Editor_State *state, Buffer_View *buffer_view, Cursor_Movement_Dir dir, bool with_shift, bool with_alt, bool with_super)
{
    if (with_shift && !buffer_view->mark.active) buffer_view_set_mark(buffer_view, buffer_view->cursor.pos);

    switch (dir)
    {
        case CURSOR_MOVE_LEFT:
        {
            if (with_alt) buffer_view->cursor.pos = cursor_pos_to_prev_start_of_word(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else if (with_super) buffer_view->cursor.pos = cursor_pos_to_indent_or_start_of_line(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else buffer_view->cursor.pos = cursor_pos_advance_char(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, -1, true);
        } break;
        case CURSOR_MOVE_RIGHT:
        {
            if (with_alt) buffer_view->cursor.pos = cursor_pos_to_next_end_of_word(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else if (with_super) buffer_view->cursor.pos = cursor_pos_to_end_of_line(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else buffer_view->cursor.pos = cursor_pos_advance_char(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, +1, true);
        } break;
        case CURSOR_MOVE_UP:
        {
            if (with_alt) buffer_view->cursor.pos = cursor_pos_to_prev_start_of_paragraph(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else if (with_super) buffer_view->cursor.pos = cursor_pos_to_start_of_buffer(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else buffer_view->cursor.pos = cursor_pos_advance_line(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, -1);
        } break;
        case CURSOR_MOVE_DOWN:
        {
            if (with_alt) buffer_view->cursor.pos = cursor_pos_to_next_start_of_paragraph(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else if (with_super) buffer_view->cursor.pos = cursor_pos_to_end_of_buffer(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            else buffer_view->cursor.pos = cursor_pos_advance_line(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, +1);
        } break;
    }

    viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
    buffer_view->cursor.blink_time = 0.0f;

    if (with_shift) buffer_view_validate_mark(buffer_view);
    else buffer_view->mark.active = false;

    return true;
}

bool action_buffer_view_prompt_submit(Editor_State *state, Buffer_View *buffer_view)
{
    Prompt_Result prompt_result = prompt_parse_result(buffer_view->buffer->text_buffer);
    if (prompt_submit(buffer_view->buffer->prompt_context, prompt_result, outer_view(buffer_view)->outer_rect, state))
        view_destroy(outer_view(buffer_view), state);
    return true;
}

bool action_buffer_view_input_char(Editor_State *state, Buffer_View *buffer_view, char c)
{
    Command *last_uncommitted_command = history_get_last_uncommitted_command(&buffer_view->buffer->history);
    if (last_uncommitted_command && last_uncommitted_command->delta_count > 0)
    {
        Delta *prev_delta = &last_uncommitted_command->deltas[last_uncommitted_command->delta_count - 1];
        if (prev_delta->kind == DELTA_INSERT_CHAR &&
            (!isalnum(prev_delta->insert_char.c) && isalnum(c)))
        {
            // If a word edge has been encountered, commit command
            history_commit_command(&buffer_view->buffer->history);
        }
    }

    history_begin_command_running(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Text insert", RUNNING_COMMAND_TEXT_INSERT);

    if (buffer_view->mark.active)
    {
        action_buffer_view_delete_selected(state, buffer_view);
    }

    text_buffer_history_insert_char(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, c, buffer_view->cursor.pos);
    if (c == '\n')
    {
        int indent_level = text_buffer_history_line_match_indent(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, buffer_view->cursor.pos.line + 1);
        buffer_view->cursor.pos = cursor_pos_clamp(
            buffer_view->buffer->text_buffer,
            (Cursor_Pos){buffer_view->cursor.pos.line + 1, indent_level});
    }
    else
    {
        buffer_view->cursor.pos = cursor_pos_advance_char(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, +1, true);
    }

    buffer_view->cursor.blink_time = 0.0f;
    viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
    return true;
}

bool action_buffer_view_delete_selected(Editor_State *state, Buffer_View *buffer_view)
{
    (void)state;
    bassert(buffer_view->mark.active);
    bassert(!cursor_pos_eq(buffer_view->mark.pos, buffer_view->cursor.pos));

    bool new_command = history_begin_command_non_interrupt(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Delete selected");

    Cursor_Pos start = cursor_pos_min(buffer_view->mark.pos, buffer_view->cursor.pos);
    Cursor_Pos end = cursor_pos_max(buffer_view->mark.pos, buffer_view->cursor.pos);
    text_buffer_history_remove_range(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, start, end);

    if (new_command) history_commit_command(&buffer_view->buffer->history);

    buffer_view->mark.active = false;
    buffer_view->cursor.pos = start;
    return true;
}

bool action_buffer_view_backspace(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->mark.active)
    {
        history_begin_command_running(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Text deletion", RUNNING_COMMAND_TEXT_DELETION);
        return action_buffer_view_delete_selected(state, buffer_view);
    }
    else
    {
        if (buffer_view->cursor.pos.line > 0 || buffer_view->cursor.pos.col > 0)
        {
            Cursor_Pos prev_cursor_pos = buffer_view->cursor.pos;
            buffer_view->cursor.pos = cursor_pos_advance_char(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, -1, true);
            char c = text_buffer_get_char(&buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
            if (c == '\n')
            {
                // Deleted line break, commit current command, before starting new one
                history_commit_command(&buffer_view->buffer->history);
            }

            history_begin_command_running(&buffer_view->buffer->history, prev_cursor_pos, buffer_view->mark, "Text deletion", RUNNING_COMMAND_TEXT_DELETION);

            text_buffer_history_remove_char(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, buffer_view->cursor.pos);

            buffer_view->cursor.blink_time = 0.0f;
            viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
        }
    }
    return true;
}

bool action_buffer_view_backspace_word(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->mark.active)
    {
        return action_buffer_view_delete_selected(state, buffer_view);
    }
    else
    {
        if (buffer_view->cursor.pos.line > 0 || buffer_view->cursor.pos.col > 0)
        {
            bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Backspace word");

            Text_Buffer *tb = &buffer_view->buffer->text_buffer;
            Cursor_Pos cursor = buffer_view->cursor.pos;
            Cursor_Pos word_start = cursor_pos_to_prev_start_of_word(*tb, cursor);
            text_buffer_history_remove_range(tb, &buffer_view->buffer->history, word_start, cursor);

            buffer_view->cursor.pos = word_start;
            buffer_view->cursor.blink_time = 0.0f;
            viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);

            if (new_command) history_commit_command(&buffer_view->buffer->history);
        }
    }

    return true;
}

bool action_buffer_view_insert_indent(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->mark.active)
    {
        return action_buffer_view_increase_indent_level(state, buffer_view);
    }
    else
    {
        bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Insert indent");
        int spaces_to_insert = INDENT_SPACES - buffer_view->cursor.pos.col % INDENT_SPACES;
        for (int i = 0; i < spaces_to_insert; i++)
        {
            text_buffer_history_insert_char(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, ' ', buffer_view->cursor.pos);
        }
        buffer_view->cursor.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, spaces_to_insert, +1, false);
        buffer_view->cursor.blink_time = 0.0f;
        viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);

        if (new_command) history_commit_command(&buffer_view->buffer->history);
    }
    return true;
}

bool action_buffer_view_decrease_indent_level(Editor_State *state, Buffer_View *buffer_view)
{
    bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Decrease indent");

    if (buffer_view->mark.active)
    {
        Cursor_Pos start = cursor_pos_min(buffer_view->mark.pos, buffer_view->cursor.pos);
        Cursor_Pos end = cursor_pos_max(buffer_view->mark.pos, buffer_view->cursor.pos);
        // If the cursor is at the start of the next linen, a multi-line operation shouldn't be done on that line
        if (end.col == 0 && end.line > 0)
        {
            end.line--;
        }
        for (int i = start.line; i <= end.line; i++)
        {
            int chars_removed = text_buffer_history_line_indent_decrease_level(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, i);
            if (i == buffer_view->mark.pos.line) buffer_view->mark.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->mark.pos, chars_removed, -1, false);
            if (i == buffer_view->cursor.pos.line) buffer_view->cursor.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, chars_removed, -1, false);
        }
    }
    else
    {
        int chars_removed = text_buffer_history_line_indent_decrease_level(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, buffer_view->cursor.pos.line);
        buffer_view->cursor.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, chars_removed, -1, false);
        viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
    }

    if (new_command) history_commit_command(&buffer_view->buffer->history);

    return true;
}

bool action_buffer_view_increase_indent_level(Editor_State *state, Buffer_View *buffer_view)
{
    bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Increase indent");

    if (buffer_view->mark.active)
    {
        Cursor_Pos start = cursor_pos_min(buffer_view->mark.pos, buffer_view->cursor.pos);
        Cursor_Pos end = cursor_pos_max(buffer_view->mark.pos, buffer_view->cursor.pos);
        // If the cursor is at the start of the next linen, a multi-line operation shouldn't be done on that line
        if (end.col == 0 && end.line > 0)
        {
            end.line--;
        }
        for (int i = start.line; i <= end.line; i++)
        {
            int chars_added = text_buffer_history_line_indent_increase_level(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, i);
            if (i == buffer_view->mark.pos.line) buffer_view->mark.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->mark.pos, chars_added, +1, false);
            if (i == buffer_view->cursor.pos.line) buffer_view->cursor.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, chars_added, +1, false);
        }
    }
    else
    {
        int chars_added = text_buffer_history_line_indent_increase_level(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, buffer_view->cursor.pos.line);
        buffer_view->cursor.pos = cursor_pos_advance_char_n(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, chars_added, +1, false);
        viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
    }

    if (new_command) history_commit_command(&buffer_view->buffer->history);

    return true;
}

bool action_buffer_view_copy_selected(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->mark.active)
    {
        Cursor_Pos start = cursor_pos_min(buffer_view->mark.pos, buffer_view->cursor.pos);
        Cursor_Pos end = cursor_pos_max(buffer_view->mark.pos, buffer_view->cursor.pos);
        if (ENABLE_OS_CLIPBOARD)
        {
            char *range = text_buffer_extract_range(&buffer_view->buffer->text_buffer, start, end);
            os_write_clipboard(range);
            free(range);
        }
        else
        {
            if (state->copy_buffer) free(state->copy_buffer);
            state->copy_buffer = text_buffer_extract_range(&buffer_view->buffer->text_buffer, start, end);
        }
        return true;
    }

    return false;
}

bool action_buffer_view_cut_selected(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->mark.active)
    {
        bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Cut selected");

        Cursor_Pos start = cursor_pos_min(buffer_view->mark.pos, buffer_view->cursor.pos);
        Cursor_Pos end = cursor_pos_max(buffer_view->mark.pos, buffer_view->cursor.pos);
        if (ENABLE_OS_CLIPBOARD)
        {
            char *range = text_buffer_extract_range(&buffer_view->buffer->text_buffer, start, end);
            os_write_clipboard(range);
            free(range);
        }
        else
        {
            if (state->copy_buffer) free(state->copy_buffer);
            state->copy_buffer = text_buffer_extract_range(&buffer_view->buffer->text_buffer, start, end);
        }

        action_buffer_view_delete_selected(state, buffer_view);

        if (new_command) history_commit_command(&buffer_view->buffer->history);
    }

    return false;
}

bool action_buffer_view_paste(Editor_State *state, Buffer_View *buffer_view)
{
    char *copy_buffer = NULL;
    if (ENABLE_OS_CLIPBOARD)
    {
        char buf[10*1024];
        os_read_clipboard(buf, sizeof(buf));
        if (strlen(buf) > 0)
        {
            copy_buffer = buf;
        }
    }
    else
    {
        copy_buffer = state->copy_buffer;
    }

    if (copy_buffer)
    {
        bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Paste");

        if (buffer_view->mark.active)
        {
            action_buffer_view_delete_selected(state, buffer_view);
        }
        Cursor_Pos start = buffer_view->cursor.pos;
        Cursor_Pos end = text_buffer_history_insert_range(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, copy_buffer, buffer_view->cursor.pos);
        buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, end);

        if (new_command) history_commit_command(&buffer_view->buffer->history);
        return true;
    }

    return false;
}

bool action_buffer_view_delete_current_line(Editor_State *state, Buffer_View *buffer_view)
{
    text_buffer_remove_line(&buffer_view->buffer->text_buffer, buffer_view->cursor.pos.line);
    buffer_view->cursor.pos = cursor_pos_to_start_of_line(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
    buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
    viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
    return true;
}

bool action_buffer_view_reload_file(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->buffer->file_path)
    {
        Text_Buffer tb;
        text_buffer_read_from_file(buffer_view->buffer->file_path, &tb);
        buffer_replace_text_buffer(buffer_view->buffer, tb);
        buffer_view->cursor.pos = cursor_pos_clamp(tb, buffer_view->cursor.pos);
    }
    return true;
}

bool action_buffer_view_prompt_save_file_as(Editor_State *state, Buffer_View *buffer_view)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_buffer_view_prompt(
        "Save as:",
        prompt_create_context_save_as(buffer_view),
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 300, 100},
        state);
    return true;
}

bool action_buffer_view_save_file(Editor_State *state, Buffer_View *buffer_view)
{
    if (buffer_view->buffer->file_path)
    {
        text_buffer_history_whitespace_cleanup(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history);
        buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
        text_buffer_write_to_file(buffer_view->buffer->text_buffer, buffer_view->buffer->file_path);
        action_save_workspace(state);
    }
    else
    {
        action_buffer_view_prompt_save_file_as(state, buffer_view);
    }
    return true;
}

bool action_buffer_view_change_zoom(Editor_State *state, Buffer_View *buffer_view, float amount)
{
    (void)state;
    buffer_view->viewport.zoom += amount;
    return true;
}

bool action_buffer_view_prompt_go_to_line(Editor_State *state, Buffer_View *buffer_view)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    create_buffer_view_prompt(
        "Go to line:",
        prompt_create_context_go_to_line(buffer_view),
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 300, 100},
        state);
    return true;
}

bool action_buffer_view_prompt_search_next(Editor_State *state, Buffer_View *buffer_view)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    View *prompt_view = create_buffer_view_prompt(
        "Search next:",
        prompt_create_context_search_next(buffer_view),
        (Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 300, 100},
        state);
    if (state->prev_search)
    {
        Text_Line current_path_line = text_line_make_f("%s", state->prev_search);
        text_buffer_insert_line(&prompt_view->bv.buffer->text_buffer, current_path_line, 1);
        prompt_view->bv.cursor.pos = cursor_pos_to_end_of_line(prompt_view->bv.buffer->text_buffer, (Cursor_Pos){1, 0});
    }
    return true;
}

bool action_buffer_view_repeat_search(Editor_State *state, Buffer_View *buffer_view)
{
    if (state->prev_search)
    {
        Cursor_Pos found_pos;
        bool found = text_buffer_search_next(&buffer_view->buffer->text_buffer, state->prev_search, buffer_view->cursor.pos, &found_pos);
        if (found)
        {
            buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, found_pos);
            viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);
            buffer_view->cursor.blink_time = 0.0f;
        }
    }
    return true;
}

bool action_buffer_view_whitespace_cleanup(Editor_State *state, Buffer_View *buffer_view)
{
    bool new_command = history_begin_command(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Whitespace cleanup");

    int cleaned_lines = text_buffer_history_whitespace_cleanup(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history);
    buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, buffer_view->cursor.pos);
    trace_log("buffer_view_whitespace_cleanup: Cleaned up %d lines", cleaned_lines);

    if (new_command) history_commit_command(&buffer_view->buffer->history);

    return true;
}

bool action_buffer_view_view_history(Editor_State *state, Buffer_View *buffer_view)
{
    v2 mouse_canvas_pos = screen_pos_to_canvas_pos(state->mouse_state.pos, state->canvas_viewport);;
    Text_Buffer history_tb = {0};
    text_buffer_append_f(&history_tb, "History. Pos: %d. Count: %d", buffer_view->buffer->history.history_pos, buffer_view->buffer->history.command_count);
    for (int command_i = 0; command_i < buffer_view->buffer->history.command_count; command_i++)
    {
        Command *c = &buffer_view->buffer->history.commands[command_i];
        text_buffer_append_f(&history_tb, "%03d: '%s' (committed: %s; initial pos: %d, %d)",
            command_i,
            c->name,
            c->committed ? "true" : "false",
            c->cursor_pos.line, c->cursor_pos.col);
        for (int delta_i = 0; delta_i < c->delta_count; delta_i++)
        {
            Delta *d = &c->deltas[delta_i];
            switch (d->kind)
            {
                case DELTA_INSERT_CHAR:
                {
                    text_buffer_append_f(&history_tb, "  %s: %c (%d, %d)",
                        DeltaKind_Str[d->kind],
                        d->insert_char.c,
                        d->insert_char.pos.line, d->insert_char.pos.col);
                } break;

                case DELTA_REMOVE_CHAR:
                {
                    text_buffer_append_f(&history_tb, "  %s: %c (%d, %d)",
                        DeltaKind_Str[d->kind],
                        d->remove_char.c,
                        d->remove_char.pos.line, d->remove_char.pos.col);
                } break;

                case DELTA_INSERT_RANGE:
                {
                    text_buffer_append_f(&history_tb, "  %s: '%s' (%d, %d -> %d, %d)",
                        DeltaKind_Str[d->kind],
                        d->insert_range.range,
                        d->insert_range.start.line, d->insert_range.start.col,
                        d->insert_range.end.line, d->insert_range.end.col);
                } break;

                case DELTA_REMOVE_RANGE:
                {
                    text_buffer_append_f(&history_tb, "  %s: '%s' (%d, %d -> %d, %d)",
                        DeltaKind_Str[d->kind],
                        d->remove_range.range,
                        d->remove_range.start.line, d->remove_range.start.col,
                        d->remove_range.end.line, d->remove_range.end.col);
                } break;
            }
        }
    }
    View *view = create_buffer_view_generic((Rect){mouse_canvas_pos.x, mouse_canvas_pos.y, 800, 400}, state);
    buffer_replace_text_buffer(view->bv.buffer, history_tb);
    return true;
}

bool action_buffer_view_undo_command(Editor_State *state, Buffer_View *buffer_view)
{
    if (history_get_last_uncommitted_command(&buffer_view->buffer->history))
    {
        history_commit_command(&buffer_view->buffer->history);
    }

    Command *command = history_get_command_to_undo(&buffer_view->buffer->history);
    if (command)
    {
        bassert(command->delta_count > 0);

        history_begin_command_non_reset(&buffer_view->buffer->history, buffer_view->cursor.pos, buffer_view->mark, "Undo action");
        command = history_get_command_to_undo(&buffer_view->buffer->history);  // Copy because begin_command could realloc

        for (int i = command->delta_count  - 1; i>= 0; i--)
        {
            Delta *delta = &command->deltas[i];
            switch (delta->kind)
            {
                case DELTA_INSERT_CHAR:
                {
                    text_buffer_history_remove_char(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, delta->insert_char.pos);
                } break;

                case DELTA_REMOVE_CHAR:
                {
                    text_buffer_history_insert_char(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, delta->remove_char.c, delta->remove_char.pos);
                } break;

                case DELTA_INSERT_RANGE:
                {
                    text_buffer_history_remove_range(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, delta->insert_range.start, delta->insert_range.end);
                } break;

                case DELTA_REMOVE_RANGE:
                {
                    text_buffer_history_insert_range(&buffer_view->buffer->text_buffer, &buffer_view->buffer->history, delta->remove_range.range, delta->remove_range.start);
                } break;
            }
        }

        history_commit_command(&buffer_view->buffer->history);
        buffer_view->buffer->history.history_pos--;

        buffer_view->mark = command->mark;
        buffer_view->cursor.pos = cursor_pos_clamp(buffer_view->buffer->text_buffer, command->cursor_pos);
        buffer_view->cursor.blink_time = 0.0f;
        viewport_snap_to_cursor(buffer_view->buffer->text_buffer, buffer_view->cursor.pos, &buffer_view->viewport, &state->render_state);

        return true;
    }
    return false;
}
