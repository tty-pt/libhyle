# Publishing @tty-pt/hyle & libhyle

## Pre-flight Checklist

1. **Verify C kernel and tests**:
   ```bash
   make clean
   make test
   ```
2. **Verify npm package and Node-API / WASM bindings**:
   ```bash
   make npm-test
   ```
3. **Verify Git working tree cleanliness**:
   ```bash
   git status -s  # Must be empty
   ```
4. **Ensure Version Parity**:
   Verify version matches across:
   - `hyle.pc`: `Version: 1.7.0`
   - `npm/hyle.pc`: `Version: 1.7.0`
   - `npm/package.json`: `"version": "1.7.0"`
   - `CHANGELOG.md`: `## [1.7.0] - <date>`

---

## 1. Build npm Package

```bash
make npm-build
```

## 2. Publish to npm Registry

```bash
cd npm
npm publish --access public
```

## 3. Git Tag & Release

```bash
git tag -a v1.7.0 -m "Release v1.7.0: Sovereign design system stylesheet and styling assets"
git push origin v1.7.0
```
