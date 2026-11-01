import { createRequire } from "node:module";
import path from "node:path";

let nativeBinding = null;
try {
  const req = createRequire(import.meta.url);
  try {
    nativeBinding = req("../build/Release/hyle_node.node");
  } catch {}
  if (!nativeBinding && typeof process !== "undefined" && process.platform && process.arch) {
    try {
      nativeBinding = req(`@ttypt/hyle-${process.platform}-${process.arch}`);
    } catch {}
  }
  if (!nativeBinding) {
    try {
      nativeBinding = req(path.join(process.cwd(), "build/Release/hyle_node.node"));
    } catch {}
  }
} catch {
  // Native addon not available; pure JS / WASM fallback
}

function getSharedWasm() {
  if (globalThis.__ttypt_shared_wasm__) {
    return globalThis.__ttypt_shared_wasm__;
  }
  try {
    const req = createRequire(import.meta.url);
    const qsysPaths = [
      path.join(path.dirname(import.meta.url.replace("file://", "")), "../../libqsys/npm/dist/index.js"),
      path.join(process.cwd(), "node_modules/@ttypt/qsys/dist/index.js"),
      "@ttypt/qsys"
    ];
    for (const p of qsysPaths) {
      try {
        const mod = req(p);
        if (mod && mod.getSharedWasm) {
          return mod.getSharedWasm();
        }
      } catch {}
    }
  } catch {}
  return null;
}

/**
 * Validates a row against hyle field descriptors using the pure C purify engine (or fallback).
 */
export function validateRow(fields, values) {
  if (nativeBinding && nativeBinding.purifyRow) {
    return nativeBinding.purifyRow(fields, values);
  }

  const wasm = getSharedWasm();
  if (wasm) {
    const errors = [];
    for (const field of fields) {
      const val = values ? values[field.name] : undefined;
      let valPtr = 0;
      if (val !== undefined && val !== null && val !== "") {
        const valBytes = Buffer.from(String(val) + "\0", "utf8");
        valPtr = wasm.malloc(valBytes.length);
        new Uint8Array(wasm.memory.buffer).set(valBytes, valPtr);
      }
      const code = wasm.hyle_wasm_validate_field(
        field.required ? 1 : 0,
        field.minLength || 0,
        field.maxLength || 0,
        valPtr
      );
      if (valPtr) wasm.free(valPtr);
      if (code === 1) {
        errors.push({ field: field.name, rule: "required", message: `Field '${field.name}' is required` });
      } else if (code === 2) {
        errors.push({ field: field.name, rule: "minLength", message: `Field '${field.name}' must be at least ${field.minLength} characters` });
      } else if (code === 3) {
        errors.push({ field: field.name, rule: "maxLength", message: `Field '${field.name}' must be at most ${field.maxLength} characters` });
      }
    }
    return errors;
  }

  // Pure JavaScript isomorphic fallback
  const errors = [];
  for (const field of fields) {
    const val = values ? values[field.name] : undefined;
    if (field.required && (val === undefined || val === null || val === "")) {
      errors.push({ field: field.name, rule: "required", message: `Field '${field.name}' is required` });
      continue;
    }
    if (val !== undefined && val !== null && val !== "") {
      const strVal = String(val);
      if (field.minLength && strVal.length < field.minLength) {
        errors.push({
          field: field.name,
          rule: "minLength",
          message: `Field '${field.name}' must be at least ${field.minLength} characters`
        });
      }
      if (field.maxLength && strVal.length > field.maxLength) {
        errors.push({
          field: field.name,
          rule: "maxLength",
          message: `Field '${field.name}' must be at most ${field.maxLength} characters`
        });
      }
      if (field.pattern) {
        try {
          const regex = new RegExp(field.pattern);
          if (!regex.test(strVal)) {
            errors.push({
              field: field.name,
              rule: "pattern",
              message: `Field '${field.name}' does not match pattern '${field.pattern}'`
            });
          }
        } catch {
          // Ignore invalid regex in fallback
        }
      }
    }
  }
  return errors;
}

export class HyleSchema {
  constructor(fields = []) {
    this.fields = fields;
  }

  addField(field) {
    this.fields.push(field);
    return this;
  }

  validate(values) {
    const errors = validateRow(this.fields, values);
    return {
      valid: errors.length === 0,
      errors
    };
  }

  toJSON() {
    return {
      fields: this.fields
    };
  }
}

/**
 * TypeScript-first schema definition helper for Next.js and frontend developers.
 * Defines schemas without needing to touch C, while executing via the C purify engine.
 */
export function defineSchema(fieldMap) {
  const fields = Object.entries(fieldMap).map(([name, config]) => ({
    name,
    type: config.type || "string",
    required: Boolean(config.required),
    minLength: config.minLength,
    maxLength: config.maxLength,
    min: config.min,
    max: config.max,
    pattern: config.pattern
  }));
  return new HyleSchema(fields);
}

/**
 * Automatically generates a TypeScript interface definition from a HyleSchema or field descriptor map.
 */
export function generateTypeScriptTypes(schemaOrFields, interfaceName = "RecordModel") {
  const fields = schemaOrFields instanceof HyleSchema ? schemaOrFields.fields : schemaOrFields;
  const lines = [`export interface ${interfaceName} {`];

  for (const f of fields) {
    let tsType = "string";
    if (f.type === "int" || f.type === "number") tsType = "number";
    else if (f.type === "bool" || f.type === "boolean") tsType = "boolean";
    else if (f.type === "array" || f.isArray) tsType = "string[]";

    const opt = f.required ? "" : "?";
    lines.push(`  ${f.name}${opt}: ${tsType};`);
  }

  lines.push("}");
  return lines.join("\n");
}

/**
 * Framework-agnostic reactive form validator (works with vanilla DOM, React, Svelte, Vue).
 * Zero external framework dependencies.
 */
export function createFormValidator(schema, initialValues = {}) {
  let values = { ...initialValues };
  let errors = schema.validate(values).errors;
  let isDirty = false;
  const subscribers = new Set();

  function notify() {
    const state = {
      values: { ...values },
      errors: [...errors],
      isValid: errors.length === 0,
      isDirty
    };
    for (const sub of subscribers) {
      try {
        sub(state);
      } catch (err) {
        console.error("Form subscriber error:", err);
      }
    }
  }

  return {
    get values() {
      return { ...values };
    },
    get errors() {
      return [...errors];
    },
    get isValid() {
      return errors.length === 0;
    },
    get isDirty() {
      return isDirty;
    },
    setField(name, value) {
      values[name] = value;
      isDirty = true;
      const res = schema.validate(values);
      errors = res.errors;
      notify();
    },
    setValues(newValues) {
      values = { ...values, ...newValues };
      isDirty = true;
      const res = schema.validate(values);
      errors = res.errors;
      notify();
    },
    validate() {
      const res = schema.validate(values);
      errors = res.errors;
      notify();
      return res;
    },
    reset(newValues = initialValues) {
      values = { ...newValues };
      errors = [];
      isDirty = false;
      notify();
    },
    subscribe(callback) {
      subscribers.add(callback);
      callback({
        values: { ...values },
        errors: [...errors],
        isValid: errors.length === 0,
        isDirty
      });
      return () => {
        subscribers.delete(callback);
      };
    }
  };
}
