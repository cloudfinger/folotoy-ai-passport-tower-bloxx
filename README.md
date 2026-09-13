<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Tower Bloxx for FoloToy AI Passport

Build a skyline one floor at a time. A square floor swings across the screen; press **OK** at the right moment to drop it onto the tower. A well-timed drop keeps the building steady, while a miss makes the next floor harder to place.

![Tower Bloxx dark cover](assets/images/tower-cover-240x320.png)

## See the game

The theme option is on the title menu. These representative interface previews are recreated from the current game layout and artwork; they are not photographs or captures of a physical device.

| Screen | Dusk theme | Daylight theme |
| --- | --- | --- |
| Title and theme setting | ![Dusk title menu with Theme: Dark selected](assets/images/readme/tower-menu-dark.png) | ![Daylight title menu with Theme: Light selected](assets/images/readme/tower-menu-light.png) |
| Stacking a tower | ![Dusk Quick Game with three square floors stacked](assets/images/readme/tower-gameplay-dark.png) | ![Daylight Quick Game showing Perfect timing](assets/images/readme/tower-gameplay-light.png) |
| Building the city | ![Dusk city map with a completed tower](assets/images/readme/tower-city-dark.png) | ![Daylight city map with a tower ready to place](assets/images/readme/tower-city-light.png) |
| The result | ![Dusk Game Over screen](assets/images/readme/tower-result-dark.png) | ![Daylight Tower Built result screen](assets/images/readme/tower-result-light.png) |

## Play

- **Quick Game:** Keep stacking and chase a higher score.
- **City Mode:** Complete towers, place them on a 4 × 4 city map, and grow the skyline.
- **UP / DOWN:** Move through the menu and city choices.
- **OK:** Select an option or drop a floor.
- **Hold OK:** Return to the game title.
- **Theme:** Choose the dusk or daylight look on the title screen. The choice is saved.

The game includes an original pixel-city cover, matching gameplay backgrounds, a looping chiptune soundtrack, and distinct cues for successful drops, perfect timing, misses, and results. It opens directly into the game after a short loading screen.

This repository contains the full game source and artwork, based on the [FoloToy AI Passport](https://github.com/FoloToy/ai-passport) project. For build and validation instructions, see the [engineering guide](docs/development/engineering/build-and-test.md). A successful build creates the complete firmware at `build/tower-bloxx-full.bin`; generated firmware files are intentionally excluded from Git. The source repository is licensed under [MIT](LICENSE).
