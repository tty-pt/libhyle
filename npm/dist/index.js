import { loadWasmModule, loadNativeAddon, getSubstrate } from "@tty-pt/stele";

const nativeBinding = loadNativeAddon(import.meta.url, "hyle");
let wasmInstance = null;

export function getHyleWasm() {
  if (wasmInstance) return wasmInstance;
  wasmInstance = loadWasmModule(new URL("./hyle.wasm", import.meta.url));
  return wasmInstance;
}

/**
 * Validates a row against hyle field descriptors strictly using C hyle_wasm_validate_field via WebAssembly
 */
export function validateRow(fields, values, options = {}) {
  if (!options.wasm && nativeBinding && nativeBinding.purifyRow) {
    return nativeBinding.purifyRow(fields, values);
  }

  const wasm = getHyleWasm();
  const mem = getSubstrate().memory;
  const enc = new TextEncoder();
  const errors = [];

  for (const field of fields) {
    const val = values ? values[field.name] : undefined;
    let valPtr = 0;
    const strVal = val !== undefined && val !== null ? String(val) : "";
    if (strVal !== "") {
      const valBytes = enc.encode(strVal + "\0");
      valPtr = wasm.exports.malloc(valBytes.length);
      new Uint8Array(mem.buffer, valPtr, valBytes.length).set(valBytes);
    }
    const code = wasm.exports.hyle_wasm_validate_field(
      field.required ? 1 : 0,
      field.minLength || 0,
      field.maxLength || 0,
      valPtr
    );
    if (valPtr) wasm.exports.free(valPtr);

    if (code === 1) {
      errors.push({ field: field.name, rule: "required", message: `Field '${field.name}' is required` });
    } else if (code === 2) {
      errors.push({ field: field.name, rule: "minLength", message: `Field '${field.name}' must be at least ${field.minLength} characters` });
    } else if (code === 3) {
      errors.push({ field: field.name, rule: "maxLength", message: `Field '${field.name}' must be at most ${field.maxLength} characters` });
    }

    if (field.pattern && strVal) {
      try {
        const regex = new RegExp(field.pattern);
        if (!regex.test(strVal)) {
          errors.push({ field: field.name, rule: "pattern", message: `Field '${field.name}' does not match pattern '${field.pattern}'` });
        }
      } catch {}
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
 * Reactive form validator factory
 */
export function createFormValidator(schema, initialValues = {}) {
  let values = { ...initialValues };
  let isDirty = false;
  let validation = schema.validate(values);
  const listeners = new Set();

  function notify() {
    const state = {
      values: { ...values },
      isDirty,
      isValid: validation.valid,
      errors: validation.errors
    };
    for (const fn of listeners) fn(state);
  }

  return {
    get values() {
      return values;
    },
    get isDirty() {
      return isDirty;
    },
    get isValid() {
      return validation.valid;
    },
    get errors() {
      return validation.errors;
    },
    setField(field, val) {
      values[field] = val;
      isDirty = true;
      validation = schema.validate(values);
      notify();
    },
    subscribe(fn) {
      listeners.add(fn);
      fn({
        values: { ...values },
        isDirty,
        isValid: validation.valid,
        errors: validation.errors
      });
      return () => listeners.delete(fn);
    }
  };
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
    else if (f.type === "bool" || f.boolean) tsType = "boolean";
    else if (f.type === "array" || f.isArray) tsType = "string[]";

    const opt = f.required ? "" : "?";
    lines.push(`  ${f.name}${opt}: ${tsType};`);
  }

  lines.push("}");
  return lines.join("\n");
}
