<p align="center">
  <img src="https://raw.githubusercontent.com/tty-pt/libhyle/main/assets/logo.png" alt="@ttypt/hyle logo" width="220" />
</p>

# @ttypt/hyle

[![npm version](https://img.shields.io/badge/npm-v0.1.0-CB3837?logo=npm)](#)
[![License: BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![TypeScript Ready](https://img.shields.io/badge/TypeScript-Ready-3178C6?logo=typescript)](#)
[![Node.js Native](https://img.shields.io/badge/Node.js-N--API-339933?logo=node.js)](#)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Reactor-654FF0?logo=webassembly)](#)
[![Zero Dependencies](https://img.shields.io/badge/dependencies-0-success)](#)

> Universal Node.js and Browser bindings for **`libhyle`**, the pure-C schema descriptor and row-level data purification engine. Features TypeScript-first declarative schema definitions, isomorphic form validation, and automated TypeScript codegen.

---

## 1. Quickstart

```bash
npm install @ttypt/hyle
```

```typescript
import { defineSchema, createFormValidator } from "@ttypt/hyle";

// 1. Author schemas in natural TypeScript (no C knowledge required)
export const MemberSchema = defineSchema({
  fullName: { type: "string", required: true, minLength: 2 },
  email: { type: "string", required: true, pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$" },
  credits: { type: "int", min: 1 }
});

// 2. Validate input on server or client with C99 purify engine:
const result = MemberSchema.validate({
  fullName: "Ana Martins",
  email: "ana@dunacowork.pt",
  credits: 5
});

console.log(result.valid); // true
console.log(result.errors); // []
```

---

## 2. Framework-Agnostic Reactive Form Controller

Provides instant keystroke validation with zero external framework dependencies (works with vanilla DOM, React, Svelte, Vue):

```typescript
import { createFormValidator } from "@ttypt/hyle";

const form = createFormValidator(MemberSchema, { fullName: "", email: "" });

// Subscribe to state updates (validity, dirty state, errors):
const unsubscribe = form.subscribe(({ values, errors, isValid, isDirty }) => {
  submitButton.disabled = !isValid;
  errorMessage.innerText = errors.map(e => e.message).join("\n");
});

// Bind to input events:
nameInput.addEventListener("input", (e) => form.setField("fullName", e.target.value));
```

---

## 3. Automated Schema-to-TypeScript Codegen (`hyle-to-ts`)

Convert shared C/JSON schema descriptors into exact TypeScript interfaces automatically:

```bash
npx hyle-to-ts --schema var/member.schema.json --name Member --out lib/types.ts
```

Outputs:
```typescript
export interface Member {
  fullName: string;
  email: string;
  credits: number;
}
```

---

## 4. Single Source of Truth Validation

- **Browser**: Keystrokes run `hyle_purify_row` in WebAssembly (`dist/hyle.wasm`).
- **Server**: Next.js Server Actions or Node API routes execute the exact same C validation logic.
- **C Backend**: `axil-hyle` runs `hyle_purify_row` natively.
- **100% Symmetry**: Zero regex divergence or validation error mismatches across client and server.

---

## 5. Building from Source

```bash
make npm-build    # Compiles native addon and WASM reactor
make npm-test     # Runs verification test suite
```

## License

BSD 2-Clause License. Copyright (c) 2026, tty-pt.
