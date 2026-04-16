# Widget UI Framework

A small, YAML-driven widget system for building moddable UI screens. Coexists with the existing `MenuSystem` and can gradually replace screens over time.

---

## Architecture

```
WidgetScreen (ICanvasState)
  └── owns vector<unique_ptr<Widget>>
        ├── WLabel           (non-focusable text)
        ├── WButton          (focusable, click callback)
        ├── WCheckbox        (focusable, toggle state)
        ├── WVerticalMenu    (focusable, scrollable item list)
        └── WScrollPanel     (container, clips children)

WidgetLoader — parses YAML screen files into widget trees
```

- **Canvas state:** `Canvas::ST_WIDGET_SCREEN = 27`
- **Source files:** `src/engine/widget/`
- **Screen definitions:** `games/doom2rpg/screens/*.yaml`

---

## Images

Widget screens can use any image registered in the engine's image map. These are the same names used in `menus.yaml` and `ui.yaml` (e.g. `main_bg`, `logo`, `menu_btn_bg`, `game_menu_background`, etc.).

Images can be used as:
- **Screen background:** `background_image` property on the screen
- **Standalone widget:** `type: image` widget
- **Button images:** `image` and `image_highlight` on button widgets

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
| `render_mode` | int | 0 | OpenGL render mode |

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

Focusable button with highlight state. Triggers an action on press.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 120x30 | Size |
| `text` | string | `""` | Button label |
| `string_id` | int | -1 | Localized string ID |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Focus highlight color |
| `action` | string | `""` | Action to dispatch on click |
| `image` | string | `""` | Normal state background image |
| `image_highlight` | string | `""` | Focused/highlight background image |

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
| `checked` | bool | false | Initial checked state |

### vertical_menu

Focusable scrollable list of selectable items. Up/Down navigates internally; at boundaries, focus moves to the next widget.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 320x160 | Size |
| `item_height` | int | 32 | Height per item in pixels |
| `visible_count` | int | 5 | Number of visible items |
| `color` | int | 0xFFFFFFFF | Text color |
| `highlight_color` | int | 0xFF3366FF | Selected item highlight |
| `items` | list | `[]` | Menu items (see below) |

Each item:

| Property | Type | Description |
|----------|------|-------------|
| `id` | string | Item identifier |
| `text` | string | Display text |
| `string_id` | int | Localized string ID |
| `action` | string | Action to dispatch on select |

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
| `selected` | int | 0 | Initially selected option index |
| `options` | list | `[]` | Available options |

Each option:

| Property | Type | Description |
|----------|------|-------------|
| `label` | string | Display text |
| `value` | int | Arbitrary value (defaults to index) |

Example:
```yaml
- type: selector
  id: window_mode
  x: 60
  y: 100
  w: 360
  h: 26
  text: "Window Mode"
  options:
    - label: "Fullscreen"
      value: 0
    - label: "Windowed"
      value: 1
    - label: "Borderless"
      value: 2
```

### scroll_panel

Container that clips children to its bounds and supports scroll.

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `id` | string | required | Unique widget ID |
| `x`, `y` | int | 0 | Position |
| `w`, `h` | int | 200x200 | Size |
| `scroll_speed` | int | 20 | Pixels per scroll step |
| `children` | list | `[]` | Child widgets |

---

## YAML Screen Format

Screens are defined in `games/doom2rpg/screens/<name>.yaml`:

```yaml
screen:
  id: my_screen
  background_color: 0xFF1A1A2E
  background_image: main_bg          # optional, drawn centered behind all widgets

  widgets:
    - type: image
      id: logo
      x: 240
      y: 30
      image: logo
      anchor: center

    - type: label
      id: title
      x: 240
      y: 16
      text: "My Screen"
      anchor: hcenter
      color: 0xFFFF7F00

    - type: vertical_menu
      id: main_menu
      x: 80
      y: 50
      w: 320
      h: 180
      item_height: 36
      visible_count: 5
      items:
        - id: option_a
          text: "First Option"
          action: do_something
        - id: back_item
          text: "Back"
          action: back

    - type: checkbox
      id: toggle
      x: 80
      y: 240
      w: 200
      h: 24
      text: "Enable Feature"

    - type: button
      id: done_btn
      x: 300
      y: 240
      w: 100
      h: 28
      text: "Done"
      action: back
```

---

## Actions

Actions are strings dispatched when a button is clicked or a menu item is selected.

| Action | Behavior |
|--------|----------|
| `goto:<screen_id>` | Push current screen onto stack, load `screens/<screen_id>.yaml` |
| `back` | Pop screen stack, or exit to previous canvas state if stack is empty |
| `close` | Same as `back` |
| `<custom>` | Dispatched to registered callback (see below) |

### Registering Custom Actions

In C++ code:

```cpp
widgetScreen.onAction("do_something", [app]() {
    // your game logic here
});
```

### Binding Widgets to Game State

Use `onScreenLoaded` to read current values into widgets and wire `onChange`/`onToggle` callbacks when a screen is loaded:

```cpp
widgetScreen.onScreenLoaded = [app](WidgetScreen* screen) {
    if (screen->screenId != "options") return;
    if (auto* w = dynamic_cast<WSlider*>(screen->findWidget("sfx_volume"))) {
        w->value = app->sound->soundFxVolume;
        w->onChange = [app](int v) { app->sound->soundFxVolume = v; };
    }
};
```

---

## Focus & Navigation

- **Keyboard/Gamepad:** Up/Down moves focus between widgets. Confirm activates. Back exits.
- **Touch:** Tap a widget to focus and activate it. Drag in scroll panels.
- **Focus order:** Follows YAML widget order (top to bottom). Only `focusable` widgets receive focus.
- **WVerticalMenu:** Captures Up/Down for internal navigation. At the first/last item, returns focus to the surrounding widgets.

---

## Opening a Widget Screen

From C++ code:

```cpp
canvas->setState(Canvas::ST_WIDGET_SCREEN);
```

Currently accessible via the **debug menu** (cheat code `3666`) → scroll to bottom → **"Widget Screen"**.

---

## File Reference

| File | Purpose |
|------|---------|
| `src/engine/widget/Widget.h/.cpp` | Base class, `WidgetInput` types |
| `src/engine/widget/WImage.h/.cpp` | Image widget |
| `src/engine/widget/WSelector.h/.cpp` | Selector (value cycler) widget |
| `src/engine/widget/WSlider.h/.cpp` | Slider widget |
| `src/engine/widget/WLabel.h/.cpp` | Label widget |
| `src/engine/widget/WButton.h/.cpp` | Button widget |
| `src/engine/widget/WCheckbox.h/.cpp` | Checkbox widget |
| `src/engine/widget/WVerticalMenu.h/.cpp` | Vertical menu widget |
| `src/engine/widget/WScrollPanel.h/.cpp` | Scroll panel container |
| `src/engine/widget/WidgetScreen.h/.cpp` | Canvas state, focus management, action dispatch |
| `src/engine/widget/WidgetLoader.h/.cpp` | YAML parser |
| `games/doom2rpg/screens/*.yaml` | Screen definitions |

---

## Notes

- All widgets support `disabled: true` in YAML. Disabled widgets render dimmed, are skipped in focus order, and ignore input. Can also be toggled at runtime via `widget->disabled = true` and calling `screen->buildFocusOrder()`.
- Coordinates are absolute pixels in 480x320 space.
- `string_id` references the localization system (same IDs used in `menus.yaml`).
- Colors are `0xAARRGGBB` hex integers.
- Text rendering uses `Localization::getLargeBuffer()` scratch buffers — always call `buf->dispose()` after use (pool has only 7 slots).
- The `games/` directory is gitignored. Screen YAML files should be added to the converter or tracked separately.
