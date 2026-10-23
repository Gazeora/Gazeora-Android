# Contributing to Gazeora for Android

Thanks for helping. The app gets better from bug reports, phone reports and code alike, and you
don't need to write code to help.

## Ways to help

- **Report a problem.**
  [Open an issue](https://github.com/Gazeora/Gazeora-Android/issues/new/choose) and pick the
  form that fits: bug report, crash or freeze, performance problem, or feature request. Name
  your phone, its Android version, the stream protocol and the codec.
- **Tell us how your phone does.** Whether the stream plays smoothly, how the VR viewer looks
  and whether USB works on a phone model is as useful as a bug report: it builds the list of
  tested phones.
- **Ask or talk over an idea** in [Discussions](https://github.com/orgs/Gazeora/discussions)
  before opening an issue, if you're not sure it's a bug or what a feature should look like.
- **Fix or build something.** See below. For anything bigger than a small fix, open an issue or
  a discussion first, so we agree on the approach before you put the time in.

Problems with capturing games or the stream itself on the PC belong to
[Gazeora for Windows](https://github.com/Gazeora/Gazeora/issues/new/choose).

Found a security problem? Don't open an issue:
[report it privately](https://github.com/Gazeora/Gazeora-Android/security/advisories/new).

## Setting up

1. Open the project in a recent Android Studio, and install what the [README](README.md#requirements)
   lists from its SDK manager: Android SDK 37, the NDK and CMake.
2. Build and run it as the README describes (`./gradlew assembleDebug`, `./gradlew installDebug`),
   with Gazeora running on a PC on the same network to stream from.
3. Run the tests: `./gradlew testDebugUnitTest` for the Java unit tests, and the native
   protocol tests from `app/src/test/cpp` with any desktop C++17 compiler (see
   [Tests](README.md#tests)).

## How the code is written

The app follows a few firm rules, so the code stays easy to find your way around:

- **MVVM and Clean Architecture.** Each feature has `presentation`, `domain` and `data` layers.
  Activities only draw the screen; ViewModels hold the state and expose it as `LiveData`; use
  cases hold the logic; repositories hide where data comes from.
- **Object-oriented, SOLID.** Fields are `private final`, classes are `final` unless built for
  inheritance, and classes depend on small interfaces. Dependencies come through the
  constructor, injected with Hilt, so they can be tested with fakes (see `app/src/test`).
- **Patterns, by name.** When a well-known pattern fits (Repository, Factory, Strategy,
  Adapter, Observer...), use it and name it in a one-line comment on the class.
- **Java, in the style around it.** Use the language a feature is already written in, and
  `@NonNull` / `@Nullable` on public methods. Never return `null` for a collection.
- **No magic values or global state.** Constants, enums or resources (`strings.xml`,
  `dimens.xml`) instead of bare numbers and strings, and no mutable statics.
- **Few, short comments.** Names should say *what* the code does; comments say *why*, in one
  plain line.

Match the code around your change. When in doubt, look at how a similar part already does it.

## Making a pull request

1. Branch from `master` and keep the change focused: one fix or feature per pull request.
2. Write commits in [Conventional Commits](https://www.conventionalcommits.org) form, one line
   each, lowercase, imperative, no period, at most 72 characters, with no body:

   ```
   feat: add profile screen with viewmodel
   fix: reconnect the stream after wi-fi drops
   ```

   Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `chore`, `style`, `build`. Make
   small commits, one logical change each.
3. Add tests for new logic in the domain and data layers, and update the README when users or
   contributors will notice the change.
4. Check that `./gradlew assembleDebug` builds and the tests pass. CI runs both on every pull
   request.
5. Open the pull request and fill in its template: what changed, and how you tested it (phone,
   Android version, protocol, codec).

A maintainer reviews every pull request. Expect questions and requests for changes; they're part
of getting the change in, not a judgment of it.

## License of contributions

Gazeora for Android is free software under the [GNU GPL version 3](LICENSE) with the additional
terms in [NOTICE](NOTICE). By opening a pull request, you confirm that you have the right to
submit the work and agree that it is licensed under those same terms. Code or libraries from
elsewhere must come under a license compatible with the GPL version 3; name it in the pull
request, and add it to [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

The Gazeora name and logo aren't covered by the license: see [TRADEMARKS.md](TRADEMARKS.md).
