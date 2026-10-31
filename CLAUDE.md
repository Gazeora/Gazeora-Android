# CLAUDE.md — Android Project

These rules are mandatory for every task in this Android project. Follow them strictly. If a rule conflicts with a request, ask before deviating.

Language: **Kotlin or Java**. Always use the language already used in the module/file you are working in. Don't mix Kotlin and Java inside one feature unless asked. Follow the matching language rules in Section 1.
Architecture: **MVVM + Clean Architecture** (same for both languages).

---

## 1. Object-Oriented Programming (Required)

All code MUST be object-oriented and follow Kotlin best practices.

### Core OOP principles
- **Encapsulation**: Keep fields/properties `private`. Expose read-only state (`StateFlow`/`val` in Kotlin, `LiveData`/getters in Java), never mutable state (`MutableStateFlow`, `MutableLiveData`, `var`, setters) outside the class.
- **Abstraction**: Depend on interfaces, not concrete classes (e.g. `UserRepository`, not `UserRepositoryImpl`).
- **Inheritance**: Only for true "is-a" relationships. Prefer **composition over inheritance**. Avoid deep `BaseActivity`/`BaseFragment` hierarchies.
- **Polymorphism**: Use `sealed class`/`sealed interface` and polymorphism instead of long `when`/`if` chains on types.

### SOLID (non-negotiable)
- **S — Single Responsibility**: One class = one job. Activities/Fragments/Composables only handle UI.
- **O — Open/Closed**: Extend by adding new classes, not editing existing ones.
- **L — Liskov Substitution**: Implementations must be fully substitutable for their interfaces.
- **I — Interface Segregation**: Small, focused interfaces.
- **D — Dependency Inversion**: Inject all dependencies with **Hilt** via constructor injection. Never create repositories, APIs, or databases manually inside classes.

### Kotlin rules
- Prefer `val` over `var`, and immutable collections.
- Use `data class` for models, `sealed class` for UI state/events/results, `object` only for stateless helpers.
- No `!!`. Handle nullability safely.
- No magic numbers or strings — use constants, `strings.xml`, `dimens.xml`, or enums.
- Use **Coroutines + Flow** for async work. No `GlobalScope`, no `AsyncTask`, no callbacks where Flow fits.
- Use `viewModelScope` / `lifecycleScope`. Run IO on `Dispatchers.IO` (injected, not hard-coded).
- Follow **DRY**, **KISS**, **YAGNI**. Keep functions short (aim for ≤ 20 lines).

### Java rules
- Make fields `private final` wherever possible. Prefer immutable objects.
- Mark classes `final` unless they are designed for inheritance.
- No public fields. Expose data through getters only; avoid setters unless truly needed.
- Use `@NonNull` / `@Nullable` (`androidx.annotation`) on all public method parameters and return values. Never return `null` for collections — return an empty list.
- Use `Optional` or null checks; never let a `NullPointerException` be possible by design.
- Use **constructor injection** with Hilt (`@Inject` on the constructor). No field injection except in Activities/Fragments where Hilt requires it.
- Models: immutable classes with `private final` fields, a constructor, getters, and proper `equals()`, `hashCode()`, `toString()`. Use a **Builder** for models with many fields.
- UI state/events: model as an abstract base class with one `final` subclass per state (`Loading`, `Success`, `Error`), or a state class with an enum status. No raw booleans like `isLoading` scattered around.
- Async work: use `LiveData` + an injected `ExecutorService`, or **RxJava** if the project already uses it. No `AsyncTask`, no raw `new Thread()`, no blocking calls on the main thread.
- ViewModels expose `LiveData<UiState>` and keep `MutableLiveData` private.
- Use `static final` constants (or `strings.xml`/`dimens.xml`) — no magic numbers or strings.
- Use enums instead of `int`/`String` constants for fixed sets of values.
- Use `try-with-resources` for anything closeable. Catch specific exceptions only; never catch and ignore `Exception`.
- Use interfaces for callbacks and listeners; no anonymous classes with large logic — use lambdas or separate classes.
- Prefer composition and interfaces over abstract base classes.
- Avoid static mutable state and utility classes holding state. Utility classes must be `final` with a `private` constructor.
- Don't use Lombok unless the project already uses it.
- Keep methods short (aim for ≤ 20 lines).

---

## 2. Architecture & Design Patterns (Required)

### Layers (Clean Architecture)
- **Presentation** → Activity/Fragment/Composable + ViewModel + UiState
- **Domain** → UseCases, domain models, repository interfaces (pure Kotlin, no Android imports)
- **Data** → Repository implementations, Retrofit APIs, Room DAOs, DTOs, mappers

Dependency direction: **Presentation → Domain ← Data**. Domain never depends on Data or Android framework.

### Patterns to use
- **MVVM** — ViewModel exposes `StateFlow<UiState>` (Kotlin) or `LiveData<UiState>` (Java); UI only observes and sends events.
- **Repository** — single source of truth for data; UI/ViewModels never call Retrofit or Room directly.
- **UseCase (Interactor)** — one business action per class with a single entry point: `operator fun invoke()` in Kotlin, `execute()` in Java.
- **Dependency Injection** — Hilt modules for all bindings.
- **Mapper** — convert DTO ↔ Entity ↔ Domain model in dedicated mapper classes/functions.
- **Factory** — for creating objects based on type/config.
- **Builder** — for complex objects with many optional parameters.
- **Adapter** — wrap third-party SDKs behind your own interfaces (and RecyclerView adapters if using XML).
- **Observer** — via `StateFlow`/`SharedFlow` (Kotlin) or `LiveData` (Java), not manual listener lists.
- **Strategy** — to swap algorithms or replace conditional logic.
- **State** — model screen states as `Loading`, `Success`, `Error` using a `sealed interface` (Kotlin) or an abstract base class with final subclasses (Java).
- **Singleton** — only through Hilt `@Singleton`, never manual singletons.

### Pattern rules
- Name the pattern in a short one-line comment on the class when it isn't obvious.
- Before writing a new feature, briefly state which classes, layers, and patterns you will use.
- Follow the UI toolkit already used in the project (Jetpack Compose or XML Views). Don't mix them in one screen.

---

## 3. Modularity & Directory Structure (Required)

Code MUST always be modular. **Create as many files and folders as needed.** More small files are always better than one large file.

- One class/interface per file; file name matches the class name.
- Organize **by feature first, then by layer**.
- Respect the existing package structure. Place new files where they logically belong; create new packages only when needed.
- Shared code goes in `core/` (or a `:core` module), never copy-pasted between features.
- Keep files short (aim for ≤ 200 lines). Split large ViewModels, Composables, and repositories.
- No circular dependencies between features or layers.
- For larger projects, prefer Gradle modules (`:core`, `:feature:login`, etc.) using the same structure.

Package layout:

```
app/src/main/java/com/<company>/<app>/
├── core/
│   ├── di/              # Hilt modules (network, database, dispatchers)
│   ├── network/         # Retrofit setup, interceptors
│   ├── database/        # Room database
│   ├── ui/              # theme, shared components
│   └── util/            # constants, extensions, Result wrapper
├── feature/
│   └── login/
│       ├── data/
│       │   ├── remote/      # API interface, DTOs
│       │   ├── local/       # DAO, entities
│       │   ├── mapper/
│       │   └── repository/  # LoginRepositoryImpl
│       ├── domain/
│       │   ├── model/
│       │   ├── repository/  # LoginRepository interface
│       │   └── usecase/
│       ├── presentation/
│       │   ├── LoginScreen / LoginFragment
│       │   ├── LoginViewModel
│       │   ├── LoginUiState
│       │   └── components/
│       └── di/              # feature Hilt module
└── MainApplication

(files use .kt for Kotlin, .java for Java — same structure for both)

app/src/test/        # unit tests, mirrors main structure
app/src/androidTest/ # instrumented/UI tests
```

Resources: keep strings in `strings.xml`, colors/dimens in resources or theme files. No hard-coded UI text.

---

## 4. Comments (Minimal & Simple)

- Write **minimal, short, easy-to-understand comments** in plain language.
- Good names should explain the code. Comment the **why**, not the **what**.
- One short line per comment where possible. No long paragraphs.
- Don't comment obvious code. No commented-out code.
- Short KDoc (Kotlin) or Javadoc (Java) only on public classes/methods when it adds clarity. One line is usually enough.

### Good
```kotlin
// Cache first so the screen loads offline
```

### Bad
```kotlin
// This function gets the user list from the repository and then it loops over
// each user and checks if the user is active and then adds it to a new list...
```

---

## 5. Git Commit Rules (Strict)

- Commit messages MUST be a **single line only**. No body, no extended description, no bullet points.
- **NEVER** add a `Co-Authored-By:` trailer.
- **NEVER** add "Generated with Claude Code", emojis, or any AI attribution.
- Use Conventional Commits: `<type>: <short summary>`
  - Types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `style`, `perf`, `build`
- Imperative mood, lowercase, no trailing period, max 72 characters.
- Always commit with a plain `-m` flag only:

```bash
git commit -m "feat: add login use case and repository"
```

### Correct
```
feat: add profile screen with viewmodel
fix: handle empty response in user repository
refactor: move network setup into hilt module
build: update compose bom version
```

### Incorrect (never do this)
```
feat: add login screen

- Added LoginViewModel
- Added LoginRepository

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>
```

- Do not use HEREDOCs or multi-line strings for commit messages.
- Small, focused commits — one logical change per commit.
- Never commit `local.properties`, keystores, API keys, or build outputs.

---

## 6. Before Finishing Any Task

- [ ] Code is object-oriented, follows SOLID and the Kotlin or Java rules for that module.
- [ ] Same language as the existing module; no unnecessary Kotlin/Java mixing.
- [ ] Follows MVVM + Clean Architecture with correct layer dependencies.
- [ ] Appropriate design patterns are used.
- [ ] Dependencies are injected with Hilt.
- [ ] Code is modular, split into small files, placed in the right package.
- [ ] No hard-coded strings or magic numbers.
- [ ] Null-safe: no `!!` in Kotlin; `@NonNull`/`@Nullable` used in Java.
- [ ] Comments are minimal and easy to understand.
- [ ] Project builds successfully (`./gradlew assembleDebug`) and tests pass.
- [ ] Commit message is a single line with no co-author or attribution.

---

## 7. Documentation Rules (Required)

The shared documentation standard for this repository is [`DOCUMENTATION_RULES.md`](./DOCUMENTATION_RULES.md). Read it before changing documentation and follow it for every task that creates, updates, or affects documented behavior.

The Android-specific guidance at the end of this section supplements it; it does not replace the existing Kotlin/Java, MVVM, Clean Architecture, Hilt, or Git rules above.

### Required workflow
- Inspect the existing documentation and relevant source code before editing docs. Treat the implementation, build configuration, tests, and shipped assets as the source of truth.
- Keep documentation accurate to the current repository state. Never invent features, commands, configuration keys, supported platforms, compatibility guarantees, performance claims, or roadmap commitments.
- Update documentation in the same change whenever behavior, setup, configuration, interfaces, dependencies, architecture, or user-visible workflows change.
- Prefer editing the existing relevant document over creating duplicates. Keep links relative where possible and verify that linked files and headings exist.
- Use concise, direct language and runnable, repository-verified examples. Mark assumptions and limitations explicitly.
- Do not remove or weaken existing coding, architecture, security, or Git rules to satisfy documentation work.

### Before finishing
- Follow the documentation review checklist in `DOCUMENTATION_RULES.md`.
- Validate Markdown structure, links, code examples, and commands where practical.
- Report exactly which documentation files changed and which checks were run. Do not claim a check passed unless it was actually run.


### Android documentation responsibilities
- Document only Android behavior that is present in this repository. Inspect Gradle files, manifest entries, source, resources, tests, and build variants before describing setup or behavior.
- Keep the root README focused on what the Android companion does, prerequisites, verified setup/build steps, supported Android versions/devices only when confirmed by project configuration, and links to deeper docs.
- Document permissions with their actual manifest declarations and runtime request behavior. Explain why each permission is needed, when it is requested, and what functionality is unavailable if it is denied. Never imply a permission is optional or mandatory without checking implementation.
- For sensor and motion tracking documentation, identify the actual sensor types, coordinate conventions, units, sampling/rate configuration, filtering, calibration, lifecycle behavior, and fallback behavior from code. Do not assume a sensor is available on every device.
- For communication with the desktop application, document the implemented transport, discovery/pairing flow, message format, versioning, connection states, reconnection behavior, security properties, and error handling. Do not claim encryption, authentication, or reliability guarantees unless implemented and verified.
- Document Android lifecycle behavior where it affects tracking, background operation, foreground services, battery use, or connection continuity. Distinguish tested behavior from platform-dependent behavior.
- Keep user-facing instructions aligned with the actual UI labels, navigation, permission prompts, and settings in the app. Store UI text in Android resources as required by the existing rules.
- Document build/test commands from the actual Gradle wrapper and modules. Verify task names and required local configuration before publishing commands.
- Do not document secrets, signing credentials, private endpoints, personal device identifiers, or sensitive debug output.

### Android-specific documentation checks
- [ ] Manifest permissions and documented permission behavior match.
- [ ] Sensor names, units, axes, coordinate frames, and lifecycle claims match the implementation.
- [ ] Desktop communication details match the actual protocol and code; unsupported guarantees are not stated.
- [ ] UI instructions match current screens and resource strings.
- [ ] Gradle commands and module names have been verified.
- [ ] Android examples use the language and UI toolkit already used in the relevant module.
