# Story progress on Linux and Windows · candidate 3104b26b

The player now reads and writes the game’s story-progress word in compiled host code on verified calls. Linux and Windows x64 under Wine exercised the change during New Game. Both reached Loading and stopped before the opening moon. Both platforms also exercised the writer while loading the earned Chapter 2 File 1. Playable startup and a successful inn save followed by a cold Continue remain release requirements. **Public binaries for this candidate remain on hold.** The earlier R48 and R55 downloads are historical builds.

The two QA packages were built from `3104b26b3189306ba793b06b2d2cdcee79ee14aa` (tree `551088be3cd2ad7330b83bb893b7070ca370627b`). The published source checkpoint `9b65a56858f38b340277bfe0fa0463b2f5acedb6` has the same three runtime files and adds a README update and a report on the earlier `27f862e0` playtests. Its normal compiler inputs and generated outputs were checked against the tested donor. The results and package hashes below belong to `3104b26b`. Both revisions descend directly from [public source 48f1b93b](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/48f1b93b1bf4b5cdf3f32b4cfec51e8d48586633).

After the local archive builds and the initial New Game test, anonymous GitHub readback verified the published source tarball at 2,883,448 bytes with SHA-256 `e55189a4c774c08ad5e87b30be4373fcd0712578fa9c579b6ff221e015c59949`. It contained 3,944 files, including 19 with executable permissions. The four-route results below extend the earlier dated report. [Published source 9b65a568](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/9b65a56858f38b340277bfe0fa0463b2f5acedb6) · [Verified source tarball](https://codeload.github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/tar.gz/9b65a56858f38b340277bfe0fa0463b2f5acedb6)

## Ordinary desktop play

These tests used the regular x86-64 players, isolated software rendering, ordinary menu inputs and fresh memory-card copies. The original US game reaches the moon and forest on New Game. On the same single-file card, the original loads room `0x3001` and shows Musashi waking in the Chapter 2 bedroom.

| Player | Route | Native reads | Native writes | Story clocks | Timing declines | Scene-counter calls, 27f → 3104 | Screen and stop |
|---|---|---:|---:|---:|---:|---:|---|
| Linux | New Game | 94 | 1 | 855 | 2 | 10 → 10 | Loading. Stopped at `8004250C` / `800338D0` before the moon. |
| Windows under Wine | New Game | 96 | 1 | 873 | 0 | 11 → 12 | Loading. Stopped at `80042568` / `800338D0` before the moon. |
| Linux | Cold Continue on File 1 | 0 | 1 | 9 | 0 | 11 → 11 | Loaded data! Stopped at `80042524` / `8003392C` before the bedroom. |
| Windows under Wine | Cold Continue on File 1 | 0 | 1 | 9 | 0 | 12 → 11 | Loaded data! Stopped at `80042510` / `8003392C` before the bedroom. |

Across the four ordinary sessions on November 23, 2026, the players completed 190 story reads and 4 writes for 1,746 credited clocks, with 2 eligible timing declines. Each cold Continue on File 1 displayed **Loaded data!** and stopped before the bedroom. Each completed story read or write accounts for nine game clocks. A timing decline means an otherwise eligible call could not reserve its quiet interval before payment. Other calls can continue on the guest CPU. The separate scene-counter operation accounts for 70 clocks per completion. The addresses in the table give the interrupted foreground and the refused interrupt callback. These counts describe execution in the software players.

Across Linux New Game and the single-file Continue test, 94 reads and two writes accounted for 864 clocks. The earlier played `27f862e0` is the comparison for both platforms. On Linux, all 176 shared New Game pictures and all 1,505 File 1 pictures matched that earlier build. The complete 2 MiB final RAM images and both complete stereo streams also matched, with 1,188,327 New Game frames and 1,106,017 File 1 frames. File 1 showed the expected 70/150 HP, 134/150 BP and 1,510 Drans, accepted Yes and displayed **Loaded data!** before stopping. The player’s final story word was 0 on New Game and 2 on File 1. Original File 1 audio was unmeasured.

In the Windows New Game comparison, the title prompt, New Game and Loading images matched `27f862e0`. The terminal dialog differed at 59 pixels. From Loading to the dialog, the Wine capture had 32,688 nonzero stereo frames on `3104b26b` and 33,178 on `27f862e0`. Timing and audio observations here apply to these captured desktop runs. On Wine File 1, four of five comparison images matched the earlier player. The terminal dialog differed at 120 pixels. From the loaded acknowledgement to that dialog, the new run had 16,577 nonzero stereo frames and the earlier run had 21,298.

## Cold Continue with four occupied files

A separate Linux `3104b26b` test accepted Continue, displayed **Loading Data** and stopped in the foreground at `8017CE28` before the slot picker opened. Both the preserved checkpoint and its working copy stayed byte-identical. With an identical card, the original US game selected File 4, loaded it and restored control outside Hotelo in Grillin’s Village, room `0x1010` at story progress 130. The single-file test in the table reached a later step. An earlier Linux player built from the public `9a45e853` source also stopped before the picker at the same address on a different four-file card.

## Native execution and its limits

The original 16-byte story reader and 16-byte writer already have matching C. The registry stays at 1,016 entries. The new host path checks the mounted game, the caller, the RAM access and enough time before the next event. Interpreter fallback continues to handle calls outside those conditions, along with other unconverted game code. An interpreter-disabled playable slice remains open.

Eighty-three pairs of focused tests compared the candidate and parent. Twenty project interrupt cases agreed on the pre-handler processor state. In both players, the scalar getter exposes the loaded result at its return instruction, which consumes the return address. A separate changed-instruction test demonstrated an early dependent read in the existing scalar fallback. Visibility at the original game’s exception entry remains unmeasured.

## Exact local QA archives

| Platform | Build ID and archive | Bytes | SHA-256 |
|---|---|---:|---|
| Linux x86-64 | `bfm-r106-clean-qa-3104b26b3.tar.gz` | 4,656,656 | `14df984a91de866ef4ba33ef09b9e7eb88df2eef32e3ee72ca04bffe32382948` |
| Windows x64 | `bfm-windows-x64-3104b26b31.zip` | 3,893,051 | `1508279575e9299b2c0cd5ff0671a5cef7167193c777538b1a9f5162aaa8ab93` |

The extracted Linux player has SHA-256 `64b4bc8394bfef1d89551473902212764e86768c372c59c9246ae9c7daeb1630`. The Windows executable has SHA-256 `c2586b93ee89143bf1e7747aad9c24f6e38fa29554e06064139ef8962db0991d`. The Linux package guard checked all 27 files and the Windows guard all 30. The archive reviews compared each package against 902 game and firmware references. They found no embedded whole reference and accounted for 575 Linux member overlaps across 24 ranges and 801 Windows member overlaps across 23 ranges. Separate checks covered compiled-code origins and sparse fragments.

## Running a local QA copy

Use your own US PlayStation disc (SLUS-00726). Keep the CUE sheet and every referenced BIN track together with their original filenames. A 2,048-byte-sector ISO omits XA audio sectors. Preserve the original memory card and use a disposable copy for testing.

- **Linux.** Extract the entire archive, open its top-level directory, and run `BFM_DISC="/path/to/disc.cue" ./launch.sh`.
- **Windows.** Extract the entire ZIP, including its DLLs. Run `launch.cmd` and select the CUE when prompted, or set `BFM_DISC` to its full path before launch.

## Separate private launcher build

The owner-only Linux launcher served `bfm-r141-mayor-berry-5c0f34854`, source `5c0f348541fc08e24d9790d59f3baab843a0ffae`. The launcher detected the update. A full readback verified the 6,105,485-byte package, SHA-256 `8c00a0fc0f4ee55dc26962d2e3c691988ab346773bcd10a735ee96b60f78e8d5`, and all 26 checked payload files. The staged R141 player earned the Mayor’s Longevity Berry, executed the maximum-HP award in host code and healed through a guest routine. The served archive independently cold-loaded the stage-130 File 4 into Grillin’s Village. In another ordinary run it awarded the Berry and the guest refill restored current HP. R141’s two Linux executables each register 16 sound and video routines covered by the public exclusions. **R141 is ineligible for public binary distribution.** [Public source exclusions](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/blob/48f1b93b1bf4b5cdf3f32b4cfec51e8d48586633/EXCLUDED.md)

## Owner device checks

| Physical owner check | 3104 Linux and Windows QA builds | Private R141 Linux |
|---|---|---|
| SDL gamepad / Steam Input default mapping in menus and gameplay | NOT RUN | NOT RUN |
| Borderless fullscreen in Steam Game Mode / Gamescope | NOT RUN | NOT RUN |
| On-screen keyboard opens automatically when each exposed text field receives focus, with password masking where applicable | NOT RUN | NOT RUN |

The Windows observations came from Wine on Linux. Physical Windows and the owner’s handheld remain untested. The private launcher has selected neither of the two 3104 QA packages.

## Full source history since R48/R55

Both branches share the 330 public commits after `ad71556b44506599fb08e654e28651d81eba7206` through `48f1b93b1bf4b5cdf3f32b4cfec51e8d48586633`. The published source checkpoint and the tested donor each add one different commit, giving each branch 331 commits since R48/R55. [Published history through 48f1b93b](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commits/48f1b93b1bf4b5cdf3f32b4cfec51e8d48586633)

### Shared public commits

- Source update 2026-10-29: 1,340 owner clean-room decompiled functions · `e0b18220472a8ade8cae7229fa419e5164071db2`
- Merge source update 2026-10-29 (1,340 owner clean-room functions) · `713295a38606bc500003f7e24011fcf33a4466d1`
- Source update 2026-10-30: 18 more owner clean-room decompiled functions · `5cd48bc13842d87ef2ed0a5ad4175a4b93499da0`
- Merge source update 2026-10-30 (18 owner clean-room functions) · `cc7b14cdb8f26f3609e9ae535d01c31aa2c58433`
- Port-source update 2026-10-30: host platform layer · `45626d1c738662cdd435a9c7bc5431234581edcb`
- Merge port-source update 2026-10-30 (host platform layer) · `5fc1cffb57051a65944910eb0307f39d42eb8538`
- Match 155 scene-object routines across six USA overlays · `180007f62c90ea9bee9bc71c6972d6e39ed2230f`
- Build and exercise native angle and movement routines from public source · `b2cc54a2abb7372c040cf6e199be0b776efd6091`
- Read verified LIST dispatch targets from the supplied disc · `6a79e717fe87c267d9832461601a4b60bd57a0d0`
- Match the opening player-state callback and direction timer · `9a275090dee6553f20ac52fadfd2d0c8b628e5e3`
- Apply PS1 texture-window masks in the CPU rasterizer · `085a746054118edab4913f0fa66c7d0d1e5ca8a0`
- Build and verify the native function library from the public registry · `016cec09c7623ce58764108b040b7f2a5314e388`
- Carry accepted polygon texture pages into following GPU primitives · `b8aa0c3a230f22a59bfe3ae43d6e6b6b9f629f15`
- Match and test the timed player event state transition · `4d1b91ff4e374852669ebb5ddf9402e0e10616b9`
- Share active SDL gamepads between gameplay and menus · `3cf8709f86480ebf9394bbeffd94d1e47efad7ea`
- Rebuild and exercise the opening camera registration and scene counter · `ecd1dcad82a7b8062a370a0368b8e3e722711f45`
- Compute triangle orientation on the shared native geometry unit · `08906cd55c2b925eb0d008ad399fa98fe014448f`
- Match resident inventory operations and test item acquisition and removal · `dc57a6d1b45e484513a2bf0db6db2ba6c4941b60`
- Accept the one-word GPU no-op used by the inn save menu · `f00fea5f1accccbd0e1b53ca417a3ff109cbbad3`
- Present the GPU display page independently of the drawing rectangle · `d45d362e645a573391654642e4ee2b5b07039e90`
- Honor borderless desktop fullscreen and preserve the original picture aspect · `7fd45d214bc65171ae850368e411bca0c8bacfe5`
- Verify Linux package provenance and discover discs in local game libraries · `756c2f73f7d30923fabc53ff5776a2244ce5ddc0`
- Rebuild the opening player event through its camera and release states · `fd7dc148f2855dc48af5d41b80bf9a20b76f4589`
- Rebuild the shop wallet and verify debit, cap and overflow behavior · `2da109f2ec473455a6a4cf1797739dfc33cb98a4`
- Replace historical work notes and upstream test excerpts with reproducible public checks · `d6d99065042cb99779c47954e263359e3630df60`
- Recover the native wallet digit formatter and both HUD packet routines · `350cb3d61570854337d57dfd14e429ecccf5910f`
- Recover the room facing selector and its signed triangle tests · `960bb68aef3e591b086abc86a87f7290598fcdbf`
- Check script-variable bus ownership before native loads and stores · `76eedac6970be2d76cb0d70d02fa1af680c20cc8`
- Recover the two title menu counters and verify their wraparound · `88dee3329728fc7c4d14e0dbee1d765c09fe23cb`
- Build the Windows player and verify multi-track paths on both hosts · `29f2d55c2d8a1c42140a682880cc5f9ff40a0de3`
- Recover the bakery purchase message lookup and selection · `a0f1771f73328796bf0bbb28ad350ee27214a0ad`
- Verify the card-menu item function and defer native execution at interrupt boundaries · `6d39578f22e04ac9374fd639a84bf57767e4f8a5`
- Declare the streaming executable digest before Linux boot uses it · `a06f06c977ff6a0111562bff22bb07fe590fdf4c`
- Keep memory-card writes bound to the owning save session · `35de4f634261f37c60cc9d7d22bceae757169e16`
- Check repeated reference spans and decoded source literals before packaging · `7688df2ae0d88379ad43b55e13a943fb9fdc1ee2`
- Match the three forest bridge trunks through their fall and settling · `92b608720efbbbd5c43e105c3ecf693b809ac2d3`
- Link current Linux and Windows source builds from the download section · `9eeb860d5dc82d621c67ff4ca37dc5a3e1b0c9bd`
- Use exact tick offsets when the source already runs at the device clock rate · `0a010dbc77762b0dad487393081539e1e5ab8b5c`
- Run device routines from the player disc and restore interrupt-driven card I/O · `a4389bcef57e0a31a0bcebd10345c317a3d1c872`
- Cache fixed resident address ranges while fetching live instruction words · `c036e819d35a08f7c9dd4829ca018c2cb9d3c825`
- Match the camera update and defer facing routines that require the guest bus · `ed04764682febeb5f542a71eb89f2e5004c90e52`
- Resolve Linux player libraries relative to the executable after relocation · `450cf6d14bfebd24c502aabb4ed21486f617ee4d`
- Run the matched wrapped-angle subtraction at each of its three guest boundaries · `d9ce63b7bfdf61e15c5bc87361adcdd08931a0fd`
- Match room camera initialization and callback selection with one shared actor · `6ccdc19c2b0c5b70823a8889abb6b54549b1b878`
- Match sixteen field movement and controller routines in their owning scene member · `d3f07b0654ff1a20236381d68bca77193e7621ca`
- Record per-player host library and ABI requirements in Linux packages · `4a1755f412782106c1d15e27c0b38d7edeb0a777`
- Find Windows discs in literal package and profile paths and preserve valid selections · `6bd1aa6983581fcac10943b644c334e68f872a39`
- Decode GPU drawing coordinates and reject oversized triangles before clipping · `ecc861401fed234131723f09c53d9b0665d524a4`
- Recover the forest floor query, cell walk, face selection and plane distance · `01cd11072cbf0f901e29f3a17dc2797179df6680`
- Match measured GPU polygon edge coverage and rectangle size fields · `3cdb3a00ddd069904a80573d460146d9b1d08ea1`
- Match the forest contact search and the plane intersection input to the normalizer · `5d7b22e564cbdd7d51c1e2cbd73e2ec4106399ab`
- Match the forest contact transform and keep it out of unchecked native dispatch · `aae402146231cc18fd0ac29f109b3d99e9729925`
- Match scene countdown and fade transitions and guard their overlay selection · `b5f4910ba34f8516941a7e270f6f40832f3c09c7`
- Match the five-way forest contact dispatcher and its classifier · `ee5c3b2daf36a816495eade51db4a2a5efafcebf`
- Match the paired phase dispatch for the return side of a scene fade · `89f3831aa61b221b943e0ae81061655563fc7dd6`
- Match forest linked-box collision and all three axis tests · `d5ce271fbbec3a4410d1c5d3e316b8e93795ba46`
- Recover the room draw dispatcher and preserve its three guest arguments · `c8e6bdaaa0091fe2895216ffe657648a8ecb9712`
- Match the sound voice release routine and model callback ordering · `ab40672a33d1f37177306d2c46019833e611bc67`
- Match room 3050 frame dispatch and honor its scene-specific state · `b19a6a3b73ecc714cf5c377755ed5306bbefaa7d`
- Check card VBlank selection, cancellation and batched completions · `aa328be4aaaca5ea17b0bb0eb65ae7218ffcf2c3`
- Match mine contact probes and correct the three-coordinate distance interface · `e69cd3244a5c01628f02476dcd95906bb48cdedc`
- Match four mine floor probes and preserve their host pointers · `17cc4a9628e0dc64830689f9c482e6778c657b89`
- Re-derive mine model surface and floor map queries · `b0f270aa0be7bbd1d46e24203402ff4ffb0548b2`
- Preserve actor handles across the two forest collision searches · `494aa466cb7459217c7b027bfd2a293e55874e83`
- Initialize interrupt auto-ack words at the selected game entry · `0f5bd9966e15dc9ba802e280c622b937172ad7a1`
- Match the moving forest segment collision query · `924361dcd1b47d610b9115e26f9d1a434a99b6ba`
- Match mine floor face intersection and dot product · `7d1ff0a0e45a37398cf487239862053a89acbb58`
- Match linked sound events and preserve channel search timing · `59c17fab62e625df594925d2f1e2f666b2894cd3`
- Match the mine model contact transform · `83e6221748d2d33595c0e45a58427d7892c4a7e7`
- Read the first matching card directory entry through game callbacks · `ff9ab165d575a9d22a06329cdc16124e93350d42`
- Match the forest slope classifier against its shared source · `545a7b29aff33ec200ce5d46cfe82fa7bae3a7e9`
- Match the mine model surface classifier · `2738c4641231ed415e23f84875caf4e11da67a27`
- Match the forest model collision query · `e34513256463f4e0bfdc4baa7669cc669e5c6b07`
- Match the four mine box contact routines · `3f2c14a498a05f85b2c81978636355e6ef66dcde`
- Read optional card transfer input as binary on Windows · `a6f7428c66fb1e895a0bb4543f3c57d902c62865`
- Match the forest model grid traversal · `3dd9420e9bc979e4e03142668ca3ca491dcd880a`
- Match the mine per-frame duration script · `6567fec85626161092c6c7c709de4fc11160a7fc`
- Read and close the authenticated memory card metadata record · `e4b1060ea47b8bc071b2cbf645a1e58147713cb6`
- Match the twenty mine slot position updates · `d01b3e75c1303952859725dd9f664de8afa87b31`
- Match the mine actor position scheduler · `40dab98cdc44906cbe873f4c824caddcd3b29b29`
- Honor the game sound fade threshold inside an interrupt · `7026224cba634727a674b679724106efefc514eb`
- Match forest movement and two-cell ground queries · `e8b355162a62f77fc01685688ddc7dd1c92469e1`
- Use the checked mine memory interface on 64-bit MIPS hosts · `1f3a0400b72e3b75974c205dd0b2828cf0e95e19`
- Match the forest actor movement query · `f43a93c0ce176699dcea98fb5c13a8a784db9ae0`
- Return through the live GPU DMA interrupt callback · `a2349a39e6ea819131f344cc0cc8fb0fc4713bf8`
- Execute the startup exchange on the full player clock · `27a3142feb57e8438425a9ccf65166210409f501`
- Match the mine actor scale and update prepasses · `4aee641a6320cd4479573b2655846516ce226c96`
- Match the mine dialog band color update · `9370f452ad3e50d6f5461fac5b620dc87a4bad0b`
- Match the conditional mine pickup handler · `f347962407ca5acb697457d14ccbde46b3698b23`
- Match the forest floor query and its contact result · `7d22b245630046a1e5593d39c5f5d80fbbdae5bb`
- Match the mine target indicator draw list · `d40671f9144b93daf9fe571ed36cb8667d6977ce`
- Resolve forest model and grid addresses on host builds · `2b2dcc5e88973ebab64e261abc69a657e28b2b08`
- Match the forest player update and action flag dispatch · `5e050a0b962f2858636e510c8a41e5df9a0a61a9`
- Report the startup exchange under its actual execution context · `83ad88296ef4e24520a6147a6387bd1d0df0cd74`
- Match the mine animation command scan · `4f92b5d0ecb541872b7b12043dcbc731bf3fbf09`
- Match the per-actor mine animation update · `0fe255435977d4752d408c63a0790e86f9ca067e`
- Match the forest player action selected by the dispatcher · `e72cb44a020a4c7e76f66cdad36036dcb369a2a9`
- Run the first game main call on the active native lane · `75ece15bfb06abb36b3e72ae0bcf0773f097de36`
- Match the two forest actor collision tests · `e746e0316d608f4eb28af5bfb932172a980ef948`
- Match the mine animation command dispatcher · `6b518b285e281f3b5d85128825610a58c4bd39cc`
- Match seven forest player action and ground query routines · `b98812691f3214dccadeed6d02bc5ed6647d383a`
- Initialize the registered game main test provider by field · `60122b83dd1978a9d8074fabd86e1851e968174d`
- Preserve the guest actor handle in the forest event callback · `61e381cfb6ae8e94b14d6a31d3308afc33175591`
- Match the mine screen effect with a four-byte guest handle · `21f0c117db2557f7ea63c12d251104002a49c0c9`
- Follow the mounted save file through its first metadata read · `67f0ebf1416d706cd906283b9678c6fd8c2c5fbe`
- Match the mine actor step toward the focus point · `6d1716eb7789e040b3b2a48aa9c9083a051c0acd`
- Match the forest player phase and three transition handlers · `8fad6747f12ba2d50b81133e37e3c42cf82f0228`
- Keep two actor counters on the guest CPU until timing matches · `08bb71cd4e3368a17bab5cb5d76f765fda436b93`
- Run seven game main instructions after the original constructor returns · `576afe1df3c3de905a78ed3a5868e924a4473cd4`
- Test two GPU DMA completions and their interrupt acknowledgments · `92d60b4846a7272c77e71f672f3217b15b021236`
- Withdraw SDK-derived routines and direct affected methods to guest execution · `c532cb8adfdb0e8b07a7ec1097b220a9e6d85956`
- Match the forest actor event checks and shared event allocator · `e6197eaa154c300c93777c92207e62293d143a77`
- Match the mine controller target update from its observed frame service · `d570408657191d8a3fefce6e7f74f974d462feb0`
- Run the startup graphics caller through its first library call · `1cbbfba6b566875464f304a908274b9161f15e18`
- Keep the forest flag read interruptible at its original instruction boundaries · `078954b24aeeeedbdf9f71a45cf6eab67a267427`
- Match the mine actor update and its two fade requests · `7b84c4bfdde552a8d6548e95fad106ea9a76b237`
- Report the clock actually driving the device epoch · `f2305668002aeb4140ef18f83730bab687bff8f5`
- Match the forest animation command reader and its selection routine · `6a7f0b8258aa54cca86cf666be5cacd5b1796a0e`
- Match the mine fade progression and its completion conditions · `14e503d4c55803a524732791222ceeabe296d7d5`
- Match the forest secondary event and its positional sound call · `c070e41a4d1425286a09ff4193bf6620536d6b9c`
- Match the mine attachment list and its per-slot position updates · `7136ad50e65f7d6430fd75344fb0c3b0dc8a7d42`
- Remove scene address tables from the public interpreter · `1649350f9e0c6724e26b67593dccee16f4575ccc`
- Use the project include directory when testing copied formatter sources · `6ad71f2ff297ea4d281ddc1349e3f219de618927`
- Always report the final GPU DMA counters on shutdown · `fa983b2ad61b59288eb728a8e989ad4b41457be1`
- Reconstruct three forest actor ground attachment routines · `4b64e33f1d14bfe55b29dd28a34931d40fd0f597`
- Reconstruct two mine frame callback dispatchers · `83aa3f0e8a9e12d15b437d19d89caa7b2a02b9fd`
- Match the game graphics startup wrapper · `cc5688bbf1fb69fc61c3cbefae8db4823b9ff8a1`
- Match the mine frame actor link update · `359bd354518a172a3cec9cd30c013eeb06e39269`
- Run the game caller through the CD library call after graphics returns · `ad60d6d1db774ab71ccca807948d416e1fd18e54`
- Resolve forest ground orientation links in the host source · `e1c5e33b9f92c7fe6620d0bf6094dd9e21bfe3a9`
- Match the mine camera update and its live owner selection · `a6da90874424975ff100301caf455b90d4414c39`
- Match the game sound startup sequence · `496030f709532efc891de02f0c0c7f6750f5cc7a`
- Match the sound record clear and preserve its per-record reads · `51152e5d86845b7727f41d55f0e1ee60fd2e6669`
- Match the game disc directory against original startup and archive entries · `357016e4d055363f498c62cb56374134723dd5dc`
- Match the eleven-state game disc reader · `fcfe6145325940a208e6f4fae3c4a2eed76281f3`
- Match the mine collision actor state dispatch · `e61d1e64cc26a6c5bfe189d8f7fad1e78847837a`
- Preserve the full word returned by the forest flag reader · `b61b26a56294ccfb9ec9427e0e099926974ce24d`
- Match the mine collision actor initializer · `4f30668430837f4830f9629b7c99738f2729310a`
- Match the game sector callback and its two dispatch tables · `dc4c865f327935800e95c3280eb711ea8e0dccd2`
- Match the mine actor visibility, motion, and collision sweeps · `37e7440c6f6b4f3ad01447b2f6148a0c76c38fa5`
- Match mine rotor creation and state dispatch · `7d713ff80584973fb7225e902693d2ddb3a70d10`
- Match the mine rotor update and sound selection · `9c12c3cc211d9e67f4ecf1e70bf5a1104b959982`
- Match the Steam Knight attachment damage callback · `d0010efa680dbe4d107a25c729996c49df7bb679`
- Model signed forest velocity updates with defined 32-bit wrapping · `8f389c5b853b17ca13af890aeaf30e79f4ab047d`
- Match game sound-stream submission and cancellation · `7bfa7b947a628f164d64a48c43a39dad07f9ec74`
- Match the forest camera vector projection wrapper · `3a23533a27e6b9b628711b1714a484bf852f4767`
- Match the Steam Knight contact that records incoming damage · `558683a151e8ee56663fdc8370a3f48a430a4d7d`
- Match the mine actor pool slot initializer · `d58661472a7814e9735d139143253546a86e5b1e`
- Match five forest actor activation functions · `24f8b77fb3c861c3a5d9e4147bc7b5bfb131790f`
- Match the tower boss damage reaction and verify its callers · `51eab8f8f88eb18edf8dbafa4a3dd8076d445a61`
- Match the SC02/0039 mine room selector with its MAIN/0012 counterpart · `538869590a9c75fc80258489f39cf2cefe577ede`
- Match the forest player model velocity transform · `1ceee70d4f7f9955a01e8780be5ddb1f31c0e273`
- Match the forest image move list to its verified sibling · `b1231ba260e009395e20c068b5a6bf2152f3b48c`
- Match the forest bridge settling effect and its particle updates · `725299abb0efc2f48e3fcc6c5e6616a978da9183`
- Match the tower effect request and its allocation of game slots · `7d75490acfbb297514319d4cb001d34c2485396f`
- Match the forest player callback and its input clearing behavior · `7ad56ac5bf071ecebcb8cd09b2f9ea3b2277900c`
- Match the forest tutorial actor dispatcher and soldier creation · `58e102f1ef22ead2893cebe18e13b837549139a5`
- Match the tower impact feedback and its effect request · `1447910a55fde10ff55c7440e7317558f3d62f4f`
- Match the forest actor ability-release callback · `39efcf47900f636c269d016db4d370f314af6a27`
- Match the game track-start state machine and share its sound-bank layout · `1bcacc2d437a91abfba65e18681d87d9ab5dad9d`
- Match the Steam Knight outer armor break and its particle requests · `9fb32313132567c41982154d98c5533a9cfaa8a4`
- Match mine room grid setup and its address relocation · `1f15cbeb5988082aed9447508e72fcce2e014506`
- Match the forest bridge player callback and its yaw adjustment · `76818c9143340d2f2f5c951f86a1882f71ed49b3`
- Match the MAIN/0010 room stream selector · `7b6bbfddafee30613dfb809d670a4eaeb6d84a67`
- Validate complete sound-memory DMA transfers · `7f879647015018949e886e4aa6bcbb29ea0f3177`
- Advance ADPCM voice history after each decoded sample · `1eed040a19130fa82ac4fbd9aaa08fc33e6d869c`
- Match the mine frame actor and field HUD routines · `8d43d1a3c1bf8989ef3272dcb3e2865192b02604`
- Match the Steam Knight particle setup and movement routines · `3c197f04f97480eb55bea918bee7ffb34f94b183`
- Match the mine backdrop painter against its loaded scene · `f0cab3df1eb970848561cfb1ad812a1b3405aca5`
- Match the forest bridge tutorial stage transitions · `471c884d31a61d24f46650d5352b36e73b411451`
- Match the Steam Knight fragment actor update and removal · `fa24b26899966f667f1c87e87ef55715ca0a7ad3`
- Match the mine stat gain display timer and movement phases · `2c297b6e1c56f742316367795e56ed14869fffa3`
- Independently match both music track row handlers · `62cac687a6be5bc1d7b05b7c116ef8bc2dd145b4`
- Match the forest projectile target queries · `aca6519e7f10a9896c58706d1d0f13689bd2db33`
- Match the Steam Knight core contact and bounded damage routines · `cca464adb4f267b35763533b75a3d2b99344aa51`
- Match the mine numeric HUD setup and drawing routines · `1cf8c907d3bacf589794a2737929f24b35ca6ef3`
- Match the forest player movement action with its controller and model state · `b2c5a778632a5c4189d4431b921a012d786f2722`
- Match the mine stat award actor and its three service wrappers · `c406867414bd3ad4add33cce4dc348fdbad0eb5a`
- Match the forest impact actor initialization, update, and expiry · `11931b4cd46696af05ca32c142b631aa100f9e3d`
- Match tower actor allocation and preserve room-specific selection · `bbca2aab63465f8b343a602cd1f32e1853cbd9db`
- Recover mine award setup and verify packed numerals across every column · `54f17c9b02cdaafddc914e574edfad86b55cd5a5`
- Match the seven tower body and marker routines · `bb20c8ba38c9f746d7064b4f4f2ac71d4a9d10c3`
- Match the forest player frame initializer and preserve ordinary dispatch · `bdfcbbac5e50bbb2b2050e1bb974d8ddf8e7cc53`
- Match the remaining seven mine award controller phases · `ba9b45d5de0c5798be65aa61d2c4d2274d198464`
- Match tower animation refresh and two opening routines · `08e81e1f12998ce988fc4fdec2415363a2110dc1`
- Match the mine model sequence evaluator and its regional table · `18c04a2224d3a40d5605fd9dd9986e362dfd43d2`
- Verify C recovery against assembly emitted in the target procedure · `cb3358755401acdd34db4bc00d37d3a0f5e8240e`
- Match four tower armor actor routines and preserve their room selection · `ff8c75122ed51b9778275e091526ce0f92b295f0`
- Match mine terrain plane distance and edge clearance tests · `ced4629e6c319ba859b2c0c872479676f17624e0`
- Recover mine award actor lookup, phases and movement services · `6590c7e110e1c6fe8db2f3ef74495f6413e33b76`
- Match the Steam Knight moving armor attachment tracker · `ab83adf5fe7550bea8132ee6231b7ad32618052a`
- Match the forest kind-four player selector and contact action · `dcbd7bbbb60e54847f2308eb641829ee64cb5b30`
- Report coverage from split public match registries · `34b2fc91ff3acbd540dca0ddefbc356e328b977d`
- Match the mine script action queue and stat award request · `ceae97d8a86bcf90ec1059f6525e0a3e5b59beb3`
- Match the Steam Knight auxiliary reset and body phase services · `09f327afc305d6b33c6abdb707f721f3eecffc84`
- Read bounded ranges of files on the mounted game disc · `8cf8cd8399db1a8a54023568096292e8957c9f2d`
- Read the forest player mesh from a mounted game disc · `43003f0dd35d34b7fb86355a0e3faeb3aa552ea7`
- Match the forest attack strength calculation · `4f3e69f899347fad37d2eb8a88c63ef86a5a79db`
- Match five Steam Knight body turning and phase routines · `0eefda43a45ccab6a9c497b71165c296417a4479`
- Interpolate polygon texture coordinates from the quantized left edge · `9472890be8811b57a0d9ccb2d65e7b46b37f1d8d`
- Use measured 12-bit steps for horizontal texture ramps · `90f2fa35a66b453b51a13d7e0751698fa045f073`
- Match the mine award model and signed local pose reads · `9a2b6af76fde9bb8305326b423dfda6aea41724b`
- Match Steam Knight joint setup, dispatch and allocation with typed model records · `15ad96932bc6f3f8ebdbe50204eb8ca5ebcc4b0c`
- Match the initial mine placement list and free actor selection · `0cc137f6e0d0128230c44f4c0c291109ca11de6c`
- Round horizontal undithered Gouraud colors at the measured precision · `77ca2f929b0fd9e64cd5ad2b0264858bcc6fffc0`
- Match Steam Knight state transitions and preserve the signed collision result · `6f8fda751fc54447e06a24cfbfbf785807238471`
- Match the mine actor world velocity and vertical step · `9b64b9ceaea3eeebee7fc7b657b523abd8c85b38`
- Match the mine door phase and model allocation · `31ce4f9e8a68b5fbe2808f957d19b1d88597b872`
- Match the mine field actor pool reset · `6a003678dbef9a00e27016afafa6dcd92b47e9c0`
- Match the fourth-stage mine field initializer in SC02/0039 · `612ccc05cb24cdcdaed05421a631ee01bc27e545`
- Match Tower body action dispatch and shared duration timer · `e9753b1d3ba6c666d64b3174d73ac2228e72f806`
- Match the mine door actor initializer in SC03 member 0116 · `64ad2e406a2d52348c85fc601b0d519bab78a980`
- Correct opaque flat-line coverage for shallow GPU segments · `8f4a9d295a432a5b9a8f5fd1bb86efbf7b0aa4b0`
- Match the room 3050 actor action dispatcher and pending movement · `45be0b40a38241c230cc97a2abcc599e54d14bc7`
- Match the Steam Knight action evaluator and four decision routines · `3656cb5f1372d0e01c243f757719f88c8659ea5d`
- Match the room 304C door actor phase dispatcher · `2e152be37b412a6eee2a03e15dfc717c8773e194`
- Match two Steam Knight action predicates and retain their callers · `726cca34090f8e0f6361e5dfb0555214d5ddf8e5`
- Match the Steam Knight joint effect and its typed actor allocation · `f09a09b48b8fa4e9d86203c82e6984619ce71630`
- Match the mine grid cell walk and its ordered face visits · `d789de43c2b976941f8699fbad325e3f7f9cf220`
- Match four Steam Knight resource routines and type their callers and test fixture · `7d40c0504983edadba945ffe7a04fded801dc632`
- Match the mine terrain flag translation from its owned overlay · `351c8b20c64510f1dd466878083f2443b07aff75`
- Match the Steam Knight actor cleanup and preserve its return type · `65363a4529936102dbb702d973be0f4ddbd56e7d`
- Match the mine terrain response routine and its cell-walker call · `1b9495952d2c7156d2fca17c6ddd25bc7c39bbec`
- Match the two Steam Knight intro countdown callbacks · `d334a239379d6e1c8f099b370f2dda32890b2fbb`
- Service pending GPU DMA and bind draw completion to the active graphics frame · `59ed3754137ecf55df044ef185868a1333b13905`
- Recognize completed GPU block uploads on the active draw interrupt · `047f04e7b897261ca18c67bdb5a9e5112d81de28`
- Match the Steam Knight turn callback and its player request helpers · `7d3a3e85f21500010e977d91aa5ce5bb192d3675`
- Recover mine actor step and its full-width terrain vector service · `cd312eb3edf5fe52d31ccf7b5030e2779c2919b3`
- Service the audio channel countdown from its mapped VBlank state · `3564df9e009f8a61a80360232dbd637cdd121d0c`
- Match six Tower camera routines and verify their transition caller · `1fb95c2b1380d78f2748913073049cd6b962e108`
- Recover the mine child spawn with the shared actor allocation declaration · `aa8c885c501879c93d1d19a229ffd37891d2b50e`
- Match camera setup and dispatch in two mine rooms · `8d796410fddb6d544f3f4cf53d4fb1820bd350b4`
- Match the mine emitter and child stage selectors · `279108c38b951fe0e59342c3223ceeffab734098`
- Deliver completed CD movie buffers through the registered callback · `bc9fdf97db8a7e46df5c169e09a3712745915837`
- Match the Steam Knight handover and its actor reset helpers · `1d15fbb47865b502b19d57292bcdd70938ae0570`
- Match the mode-zero camera updates in two mine rooms · `6f9d7d0f811676859fff406320175911e5aae685`
- Run the mounted movie output callback on completed native MDEC and GPU transfers · `8368a725e136256147cc249d0efa4c233ed97281`
- Round native MDEC color output to the nearest five-bit level · `e31823f3c066376c47f4eeaf572d6cd7a9736909`
- Register six existing forest scheduler matches in their SC01 overlay · `366fc26516941082218d03610b70e45f557adbb5`
- Match the forest enemy and its child actor in the SC01 overlay · `1ea3117804712051e45fbc5990ed0959caa2849b`
- Saturate decoded colors after signed nine-bit conversion · `8e6503d04324457d7a9dd2a9e6cb7d6e7afa849e`
- Retain fixed-point transform precision through MDEC pixel packing · `1b02587e038895ae2a7e14571882c6da1de232b8`
- Match mine emitter initialization and child model feedback · `bc9aace983b99865c9b34680d3e12393bd0da78e`
- Match four forest attack routines and align the sound command interface · `b392f05e70791ef829ac352c20daac2109b793d5`
- Match the mine child model segment query · `1e2ecb6c466833caca8a182555fe9a9273b3d8ef`
- Match the forest actor diagnostic selected by kind 218 · `c4b67103479bc3f4d4fa5a5991e521a398221624`
- Match the mine child actor setup and model allocation · `0bec1dd40b8a77ddb5ddf85d1eeb6a628bc76bc2`
- Register four forest routines and match the child allocator caller type · `cc7c0437879aa1986c8dbd644812b380dd6d72a8`
- Match the mine emitter selection and preserve the resident audio ABI · `4bb427c05d571b438edd586d1fde9f678c900972`
- Match the mine emitter projection wrapper · `1851a4208138dd453b6942b2b882984eb1587d21`
- Match the mine room 304C light actor state selector · `8472d6f2f297e9b77c16de0de23e4cade932b88c`
- Match the mine light actor first update with the resident light setter · `676d4e7fc978cb77f655c7ed9954e45dfcacaa99`
- Match both mine room 304C type 334 actor update stages · `fc8208941b2f5787f5eec30a95cace773e0c66cb`
- Match the paired mine model actor selector and setup · `874c39497c45a8cec5e2e7742d4eb32dd3ca32a0`
- Match the mine stage gate for actor type 33A · `68de17b0085e66ba67ded448db22b8347971d403`
- Match the 304C mine event dispatch and first phase · `84b0c0e0298ab48d4f9f1cf908aaa2392ef62715`
- Match the first phase of mine actor type 333 · `3970a02c86004101aeb539b739437325e1347491`
- Match the forest class 133 dispatcher alongside the Steam Knight overlay · `8d18f8a235a405313c25d1d4dbef6f7ef4fa9007`
- Match the sixth phase of mine actor type 33A · `393c6fe6a64e581ab9272896bb681ec20b015a81`
- Match the mine ring timer reset and stage update · `f1a5501a940377c4675e23fc4830679de013a609`
- Find configured memory cards and retain existing Continue saves · `272f24da11dd7e3f303579164f71960b3ea28e50`
- Retire buffered movie DMA after its decoded strip reaches memory · `df65a330a3b05b0b4e3230f00bacb58f630e81f4`
- Run the opening movie output callback as native host code · `3d09f1430af266216f1a8264b02c432aea9be489`
- Match the village porch quest actors and resident event-flag setter · `a9c96fa6792a1833bf920f012e17213518d2ea9b`
- Run title sequencer and mixer updates during Continue navigation · `fcc711a7f4efbb4a52996f4e65577052649c0fdc`
- Match the scheduled village field actor transition · `d272bf44d59c9550deeacdce9aaa55b9c001b0c4`
- Handle the mode-3 title sound callback when Continue opens · `614a69b4cb4afe81a021f528820d324a5e320871`
- Transfer buffered GPU image words to RAM on DMA channel 2 · `9a405b7aacbaed9bd04bd1aeac8669ad434fc8f6`
- Match the five-phase village actor update · `a2220e49336b3096e1a6ab5ee914e9d50f4a6f4d`
- Match the game disc audio playback report callback · `18696c57bda68b12683a17698675b71d349030d8`
- Match the saved-file status panel builder · `b6f6a6792f29c040ff43af7e7cbcc89d727924dd`
- Match the saved-file decimal formatter · `971d1565aff22e89fd7a4151071a433445f6f749`
- Match the village actor waypoint advance · `35e1edebff425f40e7695d9cfd0bd8a11cf77340`
- Verify the existing File menu glyph match and retain timed guest writes · `f4e4df3f475360f9e3b0da270657dcd62e15d1f8`
- Match the village floor selector and retain guest memory timing · `2482f23988c75e79dee9aceb0805565757cc520c`
- Restore the File 1 card payload and match the village guard at Twinpeak · `83012998d13810c7aafcb8ee15fa8e2b1e780c44`
- Match Restaurant’s Macho and the daytime Twinpeak gate · `3b569ed7014a67edec55ffaae0a2fd8df7f716a1`
- Restore the selected save through the mounted copy loop · `04b64f5831b096c14e70e7544bfd19b0eed079d6`
- Match the forest Bincho Field and Guard Lumpwood rescue · `b9e02afea744fc111f9d53a2f6c82fcb61ce5367`
- Match the village stocks actor and its story branch selection · `c1cce445c71c54f0be5918861d07cb01ece3880b`
- Match the Twinpeak Fusion throw and request queue · `5d87c58fbb74dddb9e16b292ded14062f9e7ccf6`
- Dismiss the restored file in the mounted load-menu callback · `964bca20be3ed1a1cd81f7bb0622f57b1c6bae3a`
- Read and display the mounted save rows across all four slots · `7a88d44bb4fec1e92c17ca2966ff13832f11a23a`
- Follow the completed save transaction into the mounted room initializer · `ff943a85984865df4162a224142bc834273d0dc3`
- Keep the SC01 opening scene on its own fetch path at the file callback address · `c5f6e2f0f1b86b7b6a46a6faa57cfee06fefce6d`
- Match the room change and object reset after loading File 1 · `48b8a3f98fc4622cc2df79f04d70911c2fdf4f88`
- Match File 1 room setup and three Twinpeak actor routines · `8e634d47d0e33f037cae489a53a4a70618ee31c2`
- Match the cemetery script that awards Jon's Key · `097968f288985c50bd37117ae71ef66d6a499d02`
- Restore the earned File 1 room through the owned scene callbacks · `9a45e85397060740386922d649aa6aa65a0b2135`
- Match the cemetery opening and Twinpeak contact poll · `da3739b78e46fa5c8c8868ba9f381c3ade499a1c`
- Run the registered game master fade through native clocked operations · `88ffbfa6af49ede809561905a8b9deecb73f6819`
- Match the forest rescue BP award and refill · `ea5767f90f02be97f41d3e298f3fad80935caa1a`
- Match the village actor callback after Jon’s key is used · `d9ac021f5ad826dc24e3f2d2fde3f62144b4f674`
- Run the mounted File 1 room latch in native foreground steps · `df0199542ebd403cb3f3449f452ea20d425c4862`
- Match Twinpeak Fusion contact setup and handle opening · `acd19a08abb209efd175417308d393f378a155eb`
- Match the initial Twinpeak Fusion action and document Windows and Linux testing · `654b5bf333037d96e6915d147c7fc54050d30e5c`
- Match three Twinpeak tutorial handle routines and record the two-slot Continue result · `1ee5ec4815260aca4d8777eb6112cbfe442bd439`
- Match Twinpeak tutorial setup and contact decisions, and document both player builds · `43000299588d50905e7324768155dcb9995568cd`
- Show the full launcher log path in Linux and Windows error dialogs · `f8eb164fab163bee18e14f6485b181c316dfdedf`
- Match Twinpeak coordinate checks and document both public player candidates · `782fe6f3aeac936ba83a8fd6138d20dc31e75ca9`
- Match the Twinpeak BP debit and document the four-log raft quest · `292737ec392eba20e15a6be73ac41eb726766469`
- Match repeat Fusion BP credit and document the river checkpoint · `36cb9e48c4c960753d7be144fdeb63e52b57f288`
- Match the Twinpeak cast cost and document the completed river course · `6cf4715b36c3116e2acca2fcbfdaadb0de0d668d`
- Document the earned Bracelet save and the served Shrink cast · `c5c5e8b69e35015426a2979615f7e1c92d0c4f5e`
- Register Twinpeak inventory matches and document the L-Brace appraisal · `043573bc39f83bd7db349c600aed5ce374d2911e`
- Recover Fores’s cottage conversations and document the File 4 playtest · `e0e01a5986dcc2bf85d44ce9db0cde58569c0298`
- Match the two village interaction checks for Fores and record the Steamwood Manual playthrough · `d3f30461859c49ab7eba3b00575c5f581b8098f2`
- Match the Steamwood practice valve and the Fores initializer above the village · `f960f7d2196b89a157ce15dc1b8433ecfe5a9b63`
- Match the Steamwood countdown and the plant entrance door · `f0d10e0fd4ffebae9f10a36a4884ce470d97cd29`
- Consume the opening sound transfer once and report the Steamwood playthrough · `36e596c789afe1cebf1d727bde366ca9d5d8a12d`
- Match the first numbered Steamwood valve and document Windows and Linux playback · `011164b9fb6e6e4119d7cef5703eddfda476c8ce`
- Run the opening track’s observed voice updates on its mounted clock · `c695521e3a11adbebec76f9ad24c6e5bd4c7d5e8`
- Follow the opening note release and document both desktop runs · `8d3a9b3623e776897cf87da01f1b2c06a0a7cf46`
- Run the opening player status query natively and document both real platform runs · `098b3d5af7b38f7fd8acfdd5565e366f63c2f4f6`
- Match the indoor Steamwood pressure gauge and second numbered valve selector · `2126b78c9c730a93671a0c63cc0d03676f520256`
- Match the File 1 menu-exit callback and its signed status reader · `6583f9825bad5dfbbff1703d3ec9e58cee166875`
- Run the opening sound sequence on the host and report all four player tests · `47ed86153dd2972a1a6f6cc8d6c810e685d3d996`
- Match the Steamwood timer display, failure recovery and remaining valve selectors · `f250d89811b1681d2f47ad0881f6d70454adc961`
- Dispatch primary opening events on the host and document all four player results · `a863368eb9f80b8a4e007d5632d38a8c25bcf7bd`
- Match the numbered Steamwood valve script and final cleanup · `6f5ffec0c3d8225fdc6054a976943f61f9274536`
- Document all four scene-dispatch playtests and the withheld Linux and Windows builds · `095ae49b9c9ecb98b6d10aa4d5944385fc3846c5`
- Match six Steamwood repair and story transition functions · `b3a7b1c40c3c7606c3b36ab14fb1bc6598481a89`
- Retire scene register loads and document four ordinary platform tests · `43cbe83cf5f6e130ef04b7e24ff15a9ab67c405c`
- Document Linux startup limits and two earned Steamwood rescue saves · `94174dbc4bdc877b301d173105726de7ad9686e9`
- Match the Steamwood roof-vent phase and verify the repaired File 4 · `0adc2f6ebbdc31c9478c251800a0acbc34c367ee`
- Document scene-counter starts, load timing and package checks for 94294368 · `b9978fe60b6874b8578b6bda695282e99fd291c1`
- Match the Governor’s two HP functions and verify both Wednesday saves · `5887c2a45a6f78b5790c1f36700d31621a7dc7e2`
- Record both Windows startup outcomes and verified File 4 progress · `b2abb642435c18467de766d698f012e14fd28124`
- Match the story-progress reader and writer and document both platform candidates · `2e1980c85b520e405ca2d7940a474beb87f19eb1`
- Run the scene counter in host code inside a verified scene service · `48f1b93b1bf4b5cdf3f32b4cfec51e8d48586633`

### Published source checkpoint

- Run matched story-progress reads and writes natively and document four startup tests · `9b65a56858f38b340277bfe0fa0463b2f5acedb6`

### Tested donor commit outside the public branch

- Read and update story progress in host code on authenticated foreground calls · `3104b26b3189306ba793b06b2d2cdcee79ee14aa`
