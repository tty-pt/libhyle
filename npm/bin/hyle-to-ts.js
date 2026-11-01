#!/usr/bin/env node
import fs from "node:fs";
import path from "node:path";
import { generateTypeScriptTypes } from "../dist/index.js";

const args = process.argv.slice(2);
let schemaPath = null;
let outPath = null;
let interfaceName = "RecordModel";

for (let i = 0; i < args.length; i++) {
  if (args[i] === "--schema" || args[i] === "-s") {
    schemaPath = args[++i];
  } else if (args[i] === "--out" || args[i] === "-o") {
    outPath = args[++i];
  } else if (args[i] === "--name" || args[i] === "-n") {
    interfaceName = args[++i];
  }
}

if (!schemaPath) {
  console.error("Usage: hyle-to-ts --schema <schema.json> [--out <output.ts>] [--name <InterfaceName>]");
  process.exit(1);
}

try {
  const content = fs.readFileSync(schemaPath, "utf8");
  const raw = JSON.parse(content);
  const fields = Array.isArray(raw) ? raw : raw.fields || [];

  const tsCode = generateTypeScriptTypes(fields, interfaceName);

  if (outPath) {
    fs.mkdirSync(path.dirname(outPath), { recursive: true });
    fs.writeFileSync(outPath, tsCode + "\n", "utf8");
    console.log(`Generated ${interfaceName} in ${outPath}`);
  } else {
    console.log(tsCode);
  }
} catch (err) {
  console.error("Error generating TypeScript types:", err.message);
  process.exit(1);
}
