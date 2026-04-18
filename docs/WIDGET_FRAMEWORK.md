# Widget UI Framework

A small, YAML-driven widget system for building moddable UI screens. Coexists with the existing `MenuSystem` and can gradually replace screens over time.

---

## Architecture

```
WidgetScreen (ICanvasState)
  └── owns vector<unique_ptr<Widget>>
        ├── WLabel           (non-focusable text)
        ├── WImage           (non-focusable image)
        ├── WButton          (focusable, click callback, optional animation)
        ├── WCheckbox        (focusable, toggle state)
        ├── WSlider          (focusable, numeric slider)
        ├── WSelector        (focusable, Left/Right value cycler)
        ├── WVerticalMenu    (focusable, scrollable item list)
        └── WScrollPanel     (container, clips children)

WidgetLoader — parses YAML screen files + widgets.yaml (definitions) into widget trees
```

- **Canvas state:** `Canvas::ST_WIDGET_SCREEN = 27`
- **Source files:** `src/engine/widget/`
- **Widget definitions:** `games/doom2rpg/widgets.yaml`
- **Screen definitions:** `games/doom2rpg/screens/*.yaml`

---

## Widget Definitions (widgets.yaml)

Instead of copy-pasting properties into every screen, define reusable widgets in `widgets.yaml` and reference them by name with `widget: <name>`. Per-instance properties override the definition's defaults.

```yaml
# games/doom2rpg/widgets.yaml

# Screen-level defaults applied to every screen (screen's own values override)
screen_defaults:
  background_color: 0xFF0D0D1A
  background_image: main_bg
  back_action: save_and_back

# Named widget definitions
widgets:
  # Common base for shared layout
  option_row:
    x: 60
    w: 360
    h: 28

  # Inherits from option_row, adds checkbox-specific styling
  option_checkbox:
    widget: option_row            # single-level inheritance
    type: checkbox
    check_fill_color: 0xFF00FF00

  option_slider:
    widget: option_row
    type: slider
    fill_color: 0xFF2288DD

  # Main menu text-only button with pulse animation
  main_menu_text:
    type: button
    w: 200
    h: 24
    draw_background: false
    color: 0xFFAAAAAA
    highlight_color: 0xFFFF7F00
    animate_focus: true
    animate_period_ms: 800
```

Then reference from screens:

```yaml
# games/doom2rpg/screens/options_sound.yaml
screen:
  id: options_sound
  # background_color, background_image, back_action come from screen_defaults

  widgets:
    - widget: option_checkbox    # use definition
      id: sound_enabled
      y: 60                      # override y
      text: "Sound Enabled"      # override text
      checked: true
```

**Rules:**
- Widget inheritance is single-level (A can extend B, but B cannot also extend C)
- Sequences (`items`, `options`, `children`) replace — they don't merge
- Missing `widgets.yaml` is handled gracefully (inline defaults used)

---

## Screen YAML Format

```yaml
screen:
  id: my_screen                       # unique screen ID
  background_color: 0xFF1A1A2E        # ARGB fallback color
  background_image: main_bg           # optional, from engine image registry
  back_action: save_and_back          # action dispatched on Back key (default: "back")

  widgets:
    - type: label                     # inline widget (no `widget:` reference)
      id: title
      x: 240
      y: 16
      text: "My Screen"
      anchor: hcenter
      color: 0xFFFF7F00

    - widget: option_slider           # references widgets.yaml definition
      id: volume
      y: 60
      text: "Volume"
      value: 75
```

---

## Images

Widget screens can use any image registered in the engine's image map. These are the same names used in `menus.yaml` and `ui.yaml` (e.g. `main_bg`, `logo`, `menu_btn_bg`, `game_menu_background`, etc.). See `MenuYAML.cpp` `loadUIFromYAML` for the full list.

Images can be used as:
- **Screen background:** `background_image` property on the screen
- **Standalone widget:** `type: image` widget
- **Button images:** `image` / `image_highlight` on button widgets

Image names are resolved at render time (not load time), so dynamic images like `main_bg` that can be reloaded at runtime always render correctly.

---

## Widget Types

### image

Non-focusable image display.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `image` | string | `""` | Image name from engine registry |
| `anchor` | string | `"none"` | Anchor flags |
| `render_mode` | int | 0 | OpenGL render mode (0=replace, 1=25% alpha modulate) |

### label

Non-focusable text display.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position (absolute 480x320) |
| `w`, `h` | int | 0 | Size (optional for labels) |
| `text` | string | `""` | Literal text to display |
| `string_id` | int | -1 | Localized string ID (overrides `text`) |
| `color` | int | 0xFFFFFFFF | Text color (AARRGGBB) |
| `anchor` | string | `"left"` | Anchor: `left`, `right`, `hcenter`, `vcenter`, `center`, `top`, `bottom` |

### button

Focusable button with optional image background, color rect fallback, or pure text. Supports pulse animation when focused.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 120x30 | Size |
| `text` | string | `""` | Button label |
| `string_id` | int | -1 | Localized string ID |
| `color` | int | 0xFFFFFFFF | Text color (unfocused) |
| `highlight_color` | int | 0xFF3366FF | Text color when focused |
| `bg_color` | int | 0xFF333333 | Fallback background color (when no image) |
| `border_color` | int | 0xFF666666 | Fallback border color (when no image) |
| `draw_background` | bool | true | If false and no image, draw nothing (text-only button) |
| `action` | string | `""` | Action to dispatch on click |
| `image` | string | `""` | Normal state background image name |
| `image_highlight` | string | `""` | Focused background image name |
| `render_mode` | int | 0 | Render mode for normal image (0=replace, 1=25% alpha) |
| `highlight_render_mode` | int | 0 | Render mode for highlight image |
| `animate_focus` | bool | false | Pulse text color between `color` and `highlight_color` when focused |
| `animate_period_ms` | int | 600 | Animation period in ms |
| `text_align` | string | `"center"` | Text alignment: `left`, `center`, or `right` |
| `focus_offset_x` | int | 0 | Pixel shift applied to text when focused (e.g. 14 = shift text right) |
| `focus_prefix` | string | `""` | String prepended to text when focused (e.g. `">"`) |
| `focus_cursor` | bool | false | Draw animated cursor glyph to the left of text when focused (matches classic menu behavior) |

### checkbox

Focusable toggle with check indicator.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 200x24 | Size |
| `text` | string | `""` | Label text |
| `string_id` | int | -1 | Localized string ID |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Focus highlight color |
| `check_box_color` | int | 0xFF999999 | Checkbox outline color |
| `check_fill_color` | int | 0xFF00FF00 | Checkbox fill color when checked |
| `check_size` | int | 12 | Checkbox box size in pixels |
| `check_padding` | int | 6 | Space between checkbox and label text |
| `checked` | bool | false | Initial checked state |

### slider

Focusable horizontal slider with label, track, fill bar, and numeric value display. Left/Right adjusts the value. Touch sets it directly on the track.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 300x24 | Size |
| `text` | string | `""` | Label text |
| `string_id` | int | -1 | Localized string ID |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Focus highlight color |
| `track_color` | int | 0xFF444444 | Track background color |
| `fill_color` | int | 0xFF00AA00 | Fill bar color |
| `track_height` | int | 6 | Track bar height in pixels |
| `label_width` | int | 40 | Label area width (% of widget width) |
| `value_text_width` | int | 40 | Pixels reserved for value text on the right |
| `text_padding` | int | 4 | Padding for label and value text |
| `value` | int | 50 | Initial value |
| `min` | int | 0 | Minimum value |
| `max` | int | 100 | Maximum value |
| `step` | int | 5 | Increment per Left/Right press |

### selector

Focusable value cycler. Left/Right cycles through options, displayed as `< value >` when focused. Wraps around at both ends.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 300x24 | Size |
| `text` | string | `""` | Label text |
| `string_id` | int | -1 | Localized string ID |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Focus highlight color |
| `text_padding` | int | 4 | Padding for label and value text |
| `selected` | int | 0 | Initially selected option index |
| `options` | list | `[]` | Available options |

Each option:

| Property | Type | Description |
|----------|------|-------------|
| `label` | string | Display text |
| `value` | int | Arbitrary value (defaults to index) |

### vertical_menu

Focusable scrollable list of selectable items. Up/Down navigates internally; at boundaries, focus moves to the next widget.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 320x160 | Size |
| `item_height` | int | 32 | Height per item in pixels |
| `visible_count` | int | 5 | Number of visible items |
| `item_text_padding` | int | 8 | Left padding for item text |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Selected item highlight |
| `scrollbar_color` | int | 0xFF666666 | Scrollbar indicator color |
| `scrollbar_width` | int | 3 | Scrollbar width in pixels |
| `scrollbar_offset` | int | 4 | Scrollbar distance from right edge |
| `scrollbar_min_height` | int | 8 | Minimum scrollbar height |
| `items` | list | `[]` | Menu items (see below) |

Each item:

| Property | Type | Description |
|----------|------|-------------|
| `id` | string | Item identifier |
| `text` | string | Display text |
| `string_id` | int | Localized string ID |
| `action` | string | Action to dispatch on select |

Menu items can also be populated dynamically from code via `onScreenLoaded` — see "Binding Widgets to Game State" below.

### scroll_panel

Container that clips children to its bounds and supports scroll.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 200x200 | Size |
| `scroll_speed` | int | 20 | Pixels per scroll step |
| `scrollbar_color` | int | 0xFF666666 | Scrollbar indicator color |
| `scrollbar_width` | int | 3 | Scrollbar width in pixels |
| `scrollbar_offset` | int | 4 | Scrollbar distance from right edge |
| `scrollbar_min_height` | int | 8 | Minimum scrollbar height |
| `children` | list | `[]` | Child widgets (support `widget:` references too) |

### Common properties

All widgets support:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `disabled` | bool | false | Dimmed, skipped in focus order, ignores input |
| `widget` | string | `""` | Reference to a definition in `widgets.yaml` |

---

## Actions

Actions are strings dispatched when a button is clicked or a menu item is selected.

| Action | Behavior |
|--------|----------|
| `goto:<screen_id>` | Push current screen onto stack, load `screens/<screen_id>.yaml` |
| `back` | Pop screen stack, or exit to previous canvas state if stack is empty |
| `close` | Same as `back` |
| `<custom>` | Dispatched to registered callback (see below) |

The screen's Back key (Escape / Back button) dispatches the screen's `back_action` property (default `"back"`). You can override it to save config first:

```yaml
screen:
  id: options
  back_action: save_and_back
```

### Registering Custom Actions

In C++ code:

```cpp
widgetScreen.onAction("save_and_back", [app, this]() {
    app->game->saveConfig();
    widgetScreen.dispatchAction("back");
});

widgetScreen.onAction("main_new_game", [app]() {
    app->canvas->setState(Canvas::ST_MENU);
    app->menuSystem->gotoMenu(Menus::MENU_MAIN_DIFFICULTY);
});
```

### Binding Widgets to Game State

Use `onScreenLoaded` to read current values into widgets and wire `onChange`/`onToggle` callbacks when a screen is loaded:

```cpp
widgetScreen.onScreenLoaded = [app](WidgetScreen* screen) {
    if (screen->screenId != "options_sound") return;

    if (auto* w = dynamic_cast<WSlider*>(screen->findWidget("sfx_volume"))) {
        w->value = app->sound->soundFxVolume;
        w->onChange = [app](int v) { app->sound->soundFxVolume = v; };
    }
    if (auto* w = dynamic_cast<WCheckbox*>(screen->findWidget("sound_enabled"))) {
        w->checked = app->sound->allowSounds;
        w->onToggle = [app](bool v) { app->sound->allowSounds = v; };
    }
};
```

### Dynamic Menu Items

`WVerticalMenu.items` is a public `std::vector` that can be populated from game code. Leave `items:` out of YAML and fill at load time:

```cpp
if (screen->screenId == "inventory") {
    if (auto* menu = dynamic_cast<WVerticalMenu*>(screen->findWidget("item_list"))) {
        menu->items.clear();
        for (auto& item : app->player->inventory) {
            menu->items.push_back({item.id, item.name, -1, "use_item"});
        }
        menu->onSelect = [app](int idx, const std::string& action) {
            // handle item selection
        };
    }
}
```

---

## Focus & Navigation

- **Keyboard/Gamepad:** Up/Down moves focus between widgets. Confirm activates. Back exits.
- **Touch:** Tap a widget to focus and activate it. Drag in scroll panels.
- **Focus order:** Follows YAML widget order (top to bottom). Only `focusable` widgets receive focus. Disabled widgets are skipped.
- **WVerticalMenu:** Captures Up/Down for internal navigation. At the first/last item, returns focus to the surrounding widgets.
- **Sound feedback:** Focus navigation plays `MENU_SCROLL`, Confirm/Back plays `SWITCH_EXIT`, slider Left/Right plays `MENU_SCROLL`.

---

## Opening a Widget Screen

From C++ code:

```cpp
auto* ws = static_cast<WidgetScreen*>(canvas->stateHandlers[Canvas::ST_WIDGET_SCREEN]);
ws->loadFromYAML(app, "screens/main_menu.yaml");
canvas->setState(Canvas::ST_WIDGET_SCREEN);
```

From the debug menu (cheat code `3666`):
- **"Widget Screen"** → loads `test_settings.yaml`
- **"Widget Main Menu"** → loads `main_menu.yaml`

From the existing `MenuSystem` (via YAML menu action):
```yaml
# menus.yaml
- string_id: 72
  action: widget_options     # opens screens/options.yaml
```

---

## File Reference

| File | Purpose |
|------|---------|
| `src/engine/widget/Widget.h/.cpp` | Base class, `WidgetInput` types, `disabled`/`effectiveColor` |
| `src/engine/widget/WImage.h/.cpp` | Image widget (name-resolved at render) |
| `src/engine/widget/WLabel.h/.cpp` | Label widget |
| `src/engine/widget/WButton.h/.cpp` | Button widget (image / color / text-only + focus animation) |
| `src/engine/widget/WCheckbox.h/.cpp` | Checkbox widget |
| `src/engine/widget/WSlider.h/.cpp` | Slider widget |
| `src/engine/widget/WSelector.h/.cpp` | Selector (value cycler) widget |
| `src/engine/widget/WVerticalMenu.h/.cpp` | Vertical menu widget |
| `src/engine/widget/WScrollPanel.h/.cpp` | Scroll panel container |
| `src/engine/widget/WidgetScreen.h/.cpp` | Canvas state, focus management, action dispatch, screen stack |
| `src/engine/widget/WidgetLoader.h/.cpp` | YAML parser + widget definitions resolver |
| `games/doom2rpg/widgets.yaml` | Widget definitions + screen defaults |
| `games/doom2rpg/screens/*.yaml` | Screen definitions |
| `src/converter/resources/d2-rpg/widgets.yaml` | Converter-embedded copy (auto-synced on convert) |
| `src/converter/resources/d2-rpg/screens/*.yaml` | Converter-embedded copy |

---

## Notes

- **Widget inheritance:** single-level via `widget: <name>` in a definition. Resolved at theme load time.
- **Coordinates:** absolute pixels in 480x320 space.
- **Colors:** `0xAARRGGBB` hex integers.
- **`string_id`:** references the localization system (same IDs used in `menus.yaml`).
- **Render modes:** `0` = GL_REPLACE (raw blit, no tint), `1` = GL_MODULATE at 25% alpha (dimmed, tinted by current color).
- **Text rendering:** widgets use `Localization::getLargeBuffer()` and call `buf->dispose()` after — pool has 7 slots.
- **Disabled state:** `widget->disabled = true` at runtime + `screen->buildFocusOrder()` to update focus navigation.
- **Git:** `games/doom2rpg/screens/` and `games/doom2rpg/widgets.yaml` are tracked; the rest of `games/` is ignored (regenerated by the converter).
