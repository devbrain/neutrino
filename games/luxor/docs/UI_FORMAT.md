# UI File Format Documentation

## Overview

The user interface in the MumboJumbo game engine is defined using declarative script files that construct a hierarchical scene graph of UI widgets, dialogs, buttons, text labels, and animations.

Depending on the engine generation, the UI system is organized under two formats:

1. **Classic Engine UI Format (`.ui` / `.uis`)**:
   Used across *Luxor*, *Luxor: Amun Rising*, *Luxor 2*, *Luxor HD*, *Luxor 2 HD*, *Luxor Mahjong*, and *Luxor Evolved*. Parsed by the `ui2.dll` engine subsystem.
2. **Luxor 4 Screen Definition Format (`UIScreens/*.txt`)**:
   Used in *Luxor: Quest for the Afterlife* (Luxor 4) packaged inside `.npk` / `.nhp` archives.

---

# Part 1: Classic Engine UI Format (`.ui` / `.uis`)

## 1. Syntax & Grammar

UI files use a C-like block structure with `//` comments, key-value assignments, and nested widget hierarchies.

### General Structure:

```text
// Top-level Widget Declaration
<RootWidgetName> = <WidgetClassName>
{
    // Properties
    X = <int>
    Y = <int>
    Depth = <DepthConstant>

    // Nested Child Widgets
    Child <ChildName> = <ChildWidgetClassName>
    {
        X = <int>
        Y = <int>
        Depth = <DepthConstant>
        ...
    }

    // Include external UI scripts
    Child #<FilePath>
}
```

---

## 2. Widget Class Hierarchy

All widgets derive from the base class `uiWidget` implemented in `ui2.dll`:

```
uiWidget
├── uiNonVisualWidget       (Invisible spatial container / layout group)
├── uiVisualWidget          (Base visual node rendering a sprite)
│   ├── uiButton            (Interactive 4-state clickable button)
│   ├── uiToggleButton      (Two-state checkbox / toggle switch)
│   ├── uiRadioButton       (Mutual exclusion radio button)
│   ├── uiSliderButton      (Linear draggable knob slider)
│   ├── uiTextWidget        (Text display label with fonts and justification)
│   ├── uiEntryWidget       (Single-line text input field with cursor)
│   ├── uiProgressBar       (Linear progress gauge)
│   │   └── uiProgressBar_Giza (Giza pyramid collapse progress gauge)
│   ├── uiProfileListbox    (Scrollable profile selector list)
│   ├── uiSprite            (Sprite display widget)
│   ├── uiParticleSystem    (Embedded particle system emitter)
│   ├── uiDigitalBanner     (Scrolling text banner)
│   └── uiGameLevel         (Playable map viewport, spline tracks, and level scenery)
├── uiRadioGroup            (Logical radio button controller)
├── uiFlowLayout            (Auto-flow child layout manager)
├── uiGameContainer         (Game viewport container)
└── uiDialog                (Interactive dialog with modal focus)
    └── uiAnimDialog        (Animated dialog with transition styles)
```

---

## 3. Widget Properties Reference

### Common Properties (`uiWidget` / `uiVisualWidget`)
| Property | Type | Description | Example |
|---|---|---|---|
| `X`, `Y` | Integer | Position coordinates in pixels relative to parent widget. | `X = 357`, `Y = 174` |
| `Width`, `Height` | Integer | Explicit widget bounding size. | `Width = 200`, `Height = 50` |
| `Depth` | Identifier | Z-order rendering depth layer defined in `depths.txt`, or `#Parent` to inherit. | `Depth = MenuControls`, `Depth = #Parent` |
| `Sprite` | File Path | Relative path to `.spr` descriptor. | `Sprite = data\sprites\main\button_play.spr` |
| `Flags` | Flag List | Bitwise OR combination of widget state flags. | `Flags = UIB_PRESSED \| UIB_HOVER` |

### Game Level Map Properties (`uiGameLevel` / `map.ui`)
`uiGameLevel` represents the active gameplay viewport, managing background rendering, spline path graphs for the ball train, node waypoint attributes, and layered scenery sprites.

| Property / Command | Syntax / Types | Description |
|---|---|---|
| `MapName` | String | User-facing display title of the map, displayed on HUD and victory banners (`Text_MapName`). Maximum 128 characters. |
| `GLSprite` | `<X:int> <Y:int> <Depth:id> <SpritePath:path>` | **Game Level Sprite**. Defines a scenery overlay or shadow sprite rendered at absolute coordinates `(X, Y)` at a specified layer depth from `depths.txt`. Managed in an internal linked list of 148-byte records. |
| `Path` | File Path (`.obj`) | Registers a 3D Wavefront `.obj` spline path model defining the ball track. Multiple `Path` entries define multi-track maps (0-indexed). |
| `FinalizeGraph` | Keyword (No arguments) | Compiles all registered paths into runtime spline graphs: samples 3D vertices, calculates cumulative curve distances and tangent vectors, and allocates a 48-byte node array per path with flags initialized to 0. |
| `Node` | `<PathID:int> <NodeID:int> <Flags:bitmask>` | Configures waypoint flags on the finalized graph. Flags are piped with `\|`: `SPAWN` (1), `TERMINATE` (2), `HIDDEN` (4). |

#### `Node` Flags Specification:
- **`SPAWN` (`0x01` / bit 0)**: Track entry point where new ball trains enter the playfield.
- **`TERMINATE` (`0x02` / bit 1)**: Track terminal point entering the pyramid/pit (triggers loss if balls reach it).
- **`HIDDEN` (`0x04` / bit 2)**: Visual and collision occlusion zone. Balls passing through this node are hidden or masked behind foreground scenery (e.g. entering the pyramid or traveling under tunnels and bridges).

### Button Properties (`uiButton`)
| Property | Type | Description | Example |
|---|---|---|---|
| `Hotkey` | Identifier | Keyboard shortcut triggering button activation (`KEY_ENTER`, `KEY_ESCAPE`, `KEY_SPACE`, `KEY_BACKSPACE`). | `Hotkey = KEY_ESCAPE` |
| `ClickSound` | Sound ID | Audio sound triggered on button press. | `ClickSound = button_click` |
| `HighlightSound` | Sound ID | Audio sound triggered on mouse hover. | `HighlightSound = button_hover` |
| `ParticleSystem` | File Path | Particle effect triggered when button is active. | `ParticleSystem = data\psys\btn.psys` |

### Toggle Button Properties (`uiToggleButton`)
| Property | Type | Description | Example |
|---|---|---|---|
| `Sprite` | File Path | Sprite used when unchecked / toggled OFF. | `Sprite = data\sprites\dialogs\checkbox_off.spr` |
| `Sprite2` | File Path | Sprite used when checked / toggled ON. | `Sprite2 = data\sprites\dialogs\checkbox_on.spr` |

### Slider Properties (`uiSliderButton`)
| Property | Type | Description | Example |
|---|---|---|---|
| `MinX`, `MaxX` | Integer | Horizontal travel boundary range for the slider thumb. | `MinX = 2`, `MaxX = 369` |
| `MinY`, `MaxY` | Integer | Vertical travel boundary range for vertical sliders. | `MinY = 0`, `MaxY = 100` |

### Text Properties (`uiTextWidget`)
| Property | Type | Description | Example |
|---|---|---|---|
| `Font` | File Path | Path to font definition file (`.font`). | `Font = data\fonts\dialog_header.font` |
| `Text` | String | Initial text string literal or localization key. | `Text = "OPTIONS"` |
| `Justify` | Enum | Text alignment mode (`LEFT`, `CENTER`, `RIGHT`). | `Justify = CENTER` |

### Text Entry Properties (`uiEntryWidget`)
| Property | Type | Description | Example |
|---|---|---|---|
| `Cursor` | File Path | Sprite used for blinking text caret. | `Cursor = data\fonts\dialog_body_cursor.spr` |
| `EntryWidth` | Integer | Maximum character capacity of the entry buffer. | `EntryWidth = 11` |
| `AcceptOnEnter` | Boolean | Whether pressing Enter commits the entry field. | `AcceptOnEnter = false` |

### Progress Bar Properties (`uiProgressBar`)
| Property | Type | Description | Example |
|---|---|---|---|
| `Min_Range` | Integer | Minimum progress value (default 0). | `Min_Range = 0` |
| `Max_Range` | Integer | Maximum progress value. | `Max_Range = 221` |
| `Smooth` | Boolean | Enables smooth interpolated bar filling. | `Smooth = true` |
| `Psys` | File Path | Particle effect following progress needle. | `Psys = "data\\psys\\progress.psys"` |

---

## 4. Depth Layer System (`depths.txt`)

Render ordering (Z-order) is driven by depth layer names defined in `data\uiscript\depths.txt`. Layers are drawn sequentially from top to bottom of the list:

```text
GameBackground              // Lowest game layers
GamePieceHShadow
GamePieceHidden
GameBackgroundSprites
GamePlayerBase
GamePieceNormal
GameBullet
GamePowerups
GameScores
BannerBackground            // Banner transition layers
Banner
BannerControls
HudBackground               // In-game HUD
Hud
HudControls
HudControls2
MenuBackground              // Main Menu and Dialogs
MenuControls
MenuControls2
MenuBackgroundMask
MenuDialog                  // Popup Dialog Level 1
MenuDialogControls
MenuDialog2                 // Popup Dialog Level 2 (Nested / Modal)
MenuDialog2Controls
MenuDialog3                 // Popup Dialog Level 3 (Confirmations)
MenuDialog3Controls
MenuDialog4                 // Topmost Error / Alert Dialogs
MenuDialog4Controls
```

---

## 5. Dialog Animations (`uiAnimDialog`)

Dialogs support choreographed entrance (`AnimIn`) and exit (`AnimOut`) transitions.

### Syntax:
```text
AnimIn Style = <Style>
AnimOut Style = <Style>
AnimIn Sound = <SoundID>
AnimOut Sound = <SoundID>

SubAnimCount = <N>

SubAnimIn <Index> Widget = <DialogName>.<WidgetName>
SubAnimIn <Index> Style = <Style>
SubAnimIn <Index> Time = <Milliseconds>
SubAnimIn <Index> Loc = <StartX> <StartY>
SubAnimIn <Index> AlphaStart = <0..255>
SubAnimIn <Index> AlphaTarget = <0..255>
```

### Supported Animation Styles:
- `Trivial`: Instantaneous switch without interpolation.
- `AlphaFade`: Smooth linear fade between `AlphaStart` and `AlphaTarget`.
- `SpriteMask`: Wipe transition using a grid-tiled alpha mask sprite (`SpriteHorzCount`, `SpriteVertCount`).
- `BezierLerp`: Non-linear ease-in/ease-out trajectory using Bezier control points (`BezierControls = p0x p0y p1x p1y`).

---

## 6. Real Example: Options Dialog (`menu_options.ui`)

```text
Menu_Options = uiAnimDialog
{
    X = 0
    Y = 0

    AnimIn Style = Trivial
    AnimOut Style = Trivial
    AnimIn Sound = dialog_intro
    AnimOut Sound = dialog_extro

    Child Background = uiNonVisualWidget
    {
        X = 0
        Y = 0
    }

    Child Frame = uiVisualWidget
    {
        X = 125
        Y = 110
        Depth = MenuDialog2
        Sprite = data\sprites\dialogs\dialog_1.spr

        Child Text_Title = uiTextWidget
        {
            X = 275
            Y = 15
            Depth = MenuDialog2Controls
            Font = data\fonts\dialog_header.font
            Justify = CENTER
            Text = "OPTIONS"
        }

        Child Slot_sfx = uiVisualWidget
        {
            X = 77
            Y = 95
            Depth = MenuDialog2Controls
            Sprite = data\sprites\dialogs\slider_slot.spr
            Child Slider_Effects = uiSliderButton
            {
                X = 2
                Y = 2
                Depth = MenuDialog2Controls2
                Flags = UIB_PRESSED | UIB_HOVER
                Sprite = data\sprites\dialogs\slider.spr
                MinX = 2
                MaxX = 369
            }
        }

        Child Toggle_Fullscreen = uiToggleButton
        {
            X = 190
            Y = 200
            Depth = MenuDialog2Controls
            Flags = UIB_PRESSED | UIB_HOVER
            Sprite = data\sprites\dialogs\checkbox_fullscreen_off.spr
            Sprite2 = data\sprites\dialogs\checkbox_fullscreen_on.spr
        }

        Child Button_Close = uiButton
        {
            X = 225
            Y = 291
            Depth = MenuDialog2Controls
            Sprite = data\sprites\dialogs\button_done.spr
            Flags = UIB_PRESSED | UIB_HOVER
            Hotkey = KEY_ENTER
        }
    }
}
```

---

## 7. Real Example: Game Level Map (`map.ui`)

Level maps in Luxor are also defined using the UI subsystem through the `uiGameLevel` widget:

```text
Level = uiGameLevel
{
    X = 0
    Y = 0
    Sprite = data\maps\BarqueOfTheGoddess\background.spr
    Depth = GameBackground

    MapName = "BARQUE OF THE GODDESS"

    // Load scenery sprites (pyramid shadow and pyramid foreground)
    GLSprite = 714 404 GamePieceHShadow data\sprites\game\pyramid_shadow.spr
    GLSprite = 712 402 GameBackgroundSprites data\sprites\game\pyramid_left.spr

    // Load the 3D spline track(s)
    Path = data\maps\BarqueOfTheGoddess\path.obj

    // Compile spline paths into waypoint graph
    FinalizeGraph

    // Configure waypoint occlusion (balls vanish inside the pyramid)
    Node = 0 149 HIDDEN
    Node = 0 150 HIDDEN
}
```

---

# Part 2: Luxor 4 Screen Definition Format (`UIScreens/*.txt`)

In *Luxor: Quest for the Afterlife* (Luxor 4), the UI language was restructured into screen definitions (`screen <Name> { ... }`) with event-driven actions and coordinate pairs.

### Syntax Structure:

```text
screen <ScreenName>
{
    ForceMouseCursor "<CursorName>"
    defaultWidget "<WidgetName>"
    onaction <KeyAction> "<CommandString>"

    widget <WidgetType> <WidgetName>
    {
        mins ( <X>, <Y> )
        size ( <Width>, <Height> )
        color ( <R>, <G>, <B>, <A> )
        texture "<TexturePath>"

        onaction <Action> "<Command>"
        eventToEnableOn "<EventName>"
        eventToDisableOn "<EventName>"
    }
}
```

### Key Differences from Classic Format:

1. **Root Construct**: `screen ScreenName` replaces `WidgetName = uiAnimDialog`.
2. **Coordinates**: `mins ( X, Y )` and `size ( W, H )` replace `X = ...` and `Y = ...`.
3. **Event Dispatching**: Built-in script actions (`onaction escape "popdialog"`, `onaction enter "endgame"`).
4. **Visual Types**: Keywords like `label`, `button`, `EffectEmitter` instead of `uiButton`, `uiTextWidget`.
