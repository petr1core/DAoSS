# Design System: Calm Professional SaaS

## 🎨 Design Philosophy

This Android application follows a **Calm Professional SaaS** design style, focused on productivity, clarity, and minimal visual noise. The design is inspired by modern professional tools like Linear, Notion, GitHub Mobile, and Slack.

### Core Principles
- **Minimal**: Clean, uncluttered interfaces
- **Modern**: Contemporary Material 3 design patterns
- **Confident**: Clear hierarchy and purposeful actions
- **Structured**: Consistent spacing and layout patterns
- **Productive**: Optimized for long work sessions

---

## 🎨 Color System

### Primary Colors (Cool Indigo/Blue)
- **Primary**: `#4F46E5` - Muted indigo, professional and calm
- **On Primary**: `#FFFFFF` - White text on primary
- **Primary Container**: `#E0E7FF` - Light indigo background
- **On Primary Container**: `#1E1B4B` - Dark indigo text

### Neutral Colors
- **Background**: `#FFFFFF` - Very light, near white
- **Surface**: `#FFFFFF` - Same as background for flat design
- **Surface Variant**: `#F8FAFC` - Subtle light gray for subtle separation
- **On Surface**: `#0F172A` - Dark text for readability
- **On Surface Variant**: `#475569` - Medium gray for secondary text
- **Outline**: `#CBD5E1` - Light gray for borders

### Error Colors (Subtle, Not Aggressive)
- **Error**: `#DC2626` - Red for errors
- **Error Container**: `#FEE2E2` - Light red background

### Dark Theme
- Background: `#0F172A` - Dark but not too dark
- Surface: `#1E293B` - Slightly lighter than background
- Primary: `#818CF8` - Lighter indigo for dark theme

---

## ✍ Typography

### Font Family
- **Sans-serif** - System default, clean and readable
- **sans-serif-medium** - For emphasis (medium weight)

### Text Hierarchy

#### Large Titles (Screen Headers)
- Size: `28sp` - `32sp`
- Weight: `bold`
- Color: `colorOnSurface`
- Usage: Main screen titles

#### Section Headers
- Size: `18sp` - `20sp`
- Weight: `bold` or `medium`
- Color: `colorOnSurface`
- Usage: Card titles, list item headers

#### Body Text
- Size: `14sp` - `16sp`
- Weight: `normal`
- Color: `colorOnSurface` or `colorOnSurfaceVariant`
- Usage: Descriptions, secondary information

#### Small Text / Labels
- Size: `12sp` - `14sp`
- Weight: `medium`
- Color: `colorOnSurfaceVariant`
- Usage: Badges, labels, metadata

### Line Spacing
- Body text: `1.3` multiplier for better readability
- Headers: Default spacing

---

## 🧱 Layout Principles

### Spacing System
- **Screen Padding**: `32dp` (preferred) or `24dp` (minimum)
- **Card Padding**: `16dp` - `20dp`
- **Element Spacing**: `24dp` - `32dp` between major elements
- **Small Spacing**: `8dp` - `16dp` between related elements

### Layout Style
- **Flat Layouts**: Prefer flat layouts over card-heavy UI
- **Minimal Elevation**: Use `0dp` elevation with subtle borders (`1dp` stroke)
- **Generous White Space**: Allow content to breathe
- **Consistent Margins**: `24dp` horizontal margins for list items

### Card Design
- **Corner Radius**: `12dp` - `16dp` (moderate, not too rounded)
- **Elevation**: `0dp` (flat design)
- **Border**: `1dp` stroke with `colorOutline` for subtle separation
- **Background**: `colorSurface` (white/light)

---

## 🔘 Buttons

### Primary Button
- **Style**: Filled Material 3 button
- **Corner Radius**: `16dp` - `20dp`
- **Min Height**: `56dp`
- **Text Size**: `16sp`
- **Color**: Primary color
- **Usage**: Main actions (Create, Save, Submit)

### Secondary Actions
- **Style**: Text buttons or outlined buttons
- **Usage**: Cancel, Back, Less important actions

### Extended FAB
- **Corner Radius**: `16dp`
- **Elevation**: `2dp` (minimal)
- **Margin**: `24dp` from edges
- **Usage**: Primary action on list screens

---

## 📝 Inputs & Forms

### Text Fields
- **Style**: Material 3 Outlined Text Fields
- **No Icons**: Avoid unnecessary icons (only when truly needed)
- **Clear Labels**: Descriptive hints
- **Spacing**: `24dp` between fields
- **Error States**: Subtle, not aggressive

### Form Layout
- **Padding**: `32dp` from screen edges
- **Vertical Flow**: Logical top-to-bottom flow
- **No Cards**: Flat layout, no card containers around forms

---

## 📱 Component Guidelines

### List Items
- **Style**: Flat cards with `1dp` border
- **Padding**: `16dp` - `20dp` internal
- **Margins**: `24dp` horizontal, `3dp` - `4dp` vertical
- **Corner Radius**: `12dp`
- **Elevation**: `0dp`

### Icons
- **Size**: `24dp` for standard icons
- **Color**: `colorPrimary` for primary actions, `colorOnSurfaceVariant` for secondary
- **Usage**: Minimal, only when necessary

### Badges / Tags
- **Style**: Rounded rectangles with `12dp` corner radius
- **Padding**: `8dp` - `10dp` horizontal, `4dp` - `6dp` vertical
- **Background**: `colorSecondaryContainer`
- **Text**: `12sp`, `medium` weight, uppercase

### Empty States
- **Text**: `18sp` bold for title, `14sp` for description
- **Color**: `colorOnSurfaceVariant` (subtle, not prominent)
- **No Icons**: Or very minimal icons with low opacity

---

## 🚫 What to Avoid

- ❌ Bright neon colors
- ❌ Excessive shadows (max `2dp` elevation)
- ❌ Heavy gradients
- ❌ Overuse of cards (prefer flat layouts)
- ❌ Playful or game-like UI
- ❌ Banking/fintech aggressive aesthetics
- ❌ Decorative fonts
- ❌ Centered forms (except auth screens)
- ❌ Too many buttons on one screen

---

## 📦 Design Tokens

### Spacing Scale
```
8dp   - Tiny spacing
16dp  - Small spacing
24dp  - Medium spacing (preferred)
32dp  - Large spacing (preferred)
```

### Corner Radius Scale
```
12dp  - Small elements (badges, small cards)
16dp  - Medium elements (buttons, standard cards)
20dp  - Large elements (large buttons)
```

### Elevation Scale
```
0dp   - Flat elements (preferred)
1dp   - Subtle separation
2dp   - FAB, floating elements
```

### Typography Scale
```
12sp  - Small text, badges
14sp  - Body text, descriptions
16sp  - Standard text, buttons
18sp  - Section headers
20sp  - Card titles
28sp  - Screen titles
```

---

## 🎯 Screen-Specific Guidelines

### Authentication Screens
- Centered layout is acceptable
- Clean, minimal form
- Clear call-to-action button
- Subtle branding

### List Screens
- Flat list items with borders
- Generous spacing between items
- Clear primary action (FAB)
- Empty states with helpful text

### Form Screens
- Flat layout, no card containers
- Clear field labels
- Logical vertical flow
- Prominent submit button

### Detail Screens
- Clear hierarchy
- Grouped related information
- Secondary actions clearly separated
- Consistent with list item styling

---

## 🌓 Dark Theme

All colors adapt automatically via Material 3 theming. Dark theme maintains the same principles:
- Slightly darker backgrounds
- Adjusted contrast ratios
- Same spacing and layout rules
- Consistent component styling

---

## ✅ Checklist for New Screens

- [ ] Uses `32dp` or `24dp` padding
- [ ] Flat layout (no unnecessary cards)
- [ ] `0dp` elevation with `1dp` border if needed
- [ ] Consistent typography hierarchy
- [ ] Primary color only for primary actions
- [ ] Generous white space
- [ ] Clear visual hierarchy
- [ ] No excessive icons
- [ ] Subtle, professional colors
- [ ] Follows spacing scale

