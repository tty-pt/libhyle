import test from "node:test";
import assert from "node:assert/strict";
import { validateRow, HyleSchema, defineSchema, createFormValidator, generateTypeScriptTypes } from "../dist/index.js";

test("validateRow detects required and minLength violations via hyle_purify", () => {
  const schema = new HyleSchema([
    { name: "username", required: true, minLength: 3 },
    { name: "email", required: true, pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$" },
    { name: "bio", required: false, minLength: 5 }
  ]);

  const res1 = schema.validate({});
  assert.equal(res1.valid, false);
  assert.ok(res1.errors.length >= 2, "Should report at least 2 required errors");
  assert.ok(res1.errors.some((e) => e.field === "username" && e.rule === "required"));

  const res2 = schema.validate({ username: "alice", email: "alice@example.com", bio: "Engineer" });
  assert.equal(res2.valid, true);
  assert.equal(res2.errors.length, 0);
});

test("defineSchema enables TypeScript-first schema declaration without touching C", () => {
  const memberSchema = defineSchema({
    fullName: { type: "string", required: true, minLength: 2 },
    email: { type: "string", required: true, pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$" },
    credits: { type: "int", min: 1 }
  });

  const resInvalid = memberSchema.validate({ fullName: "A", email: "invalid" });
  assert.equal(resInvalid.valid, false);
  assert.ok(resInvalid.errors.some((e) => e.field === "fullName" && (e.rule === "minLength" || e.rule === "min_length")));
  assert.ok(resInvalid.errors.some((e) => e.field === "email" && e.rule === "pattern"));

  const resValid = memberSchema.validate({ fullName: "Ana Martins", email: "ana@example.com" });
  assert.equal(resValid.valid, true);
});

test("createFormValidator provides framework-agnostic reactive form subscriptions", () => {
  const schema = defineSchema({
    seatNumber: { type: "string", required: true, minLength: 3 }
  });

  const form = createFormValidator(schema, { seatNumber: "" });
  assert.equal(form.isValid, false);
  assert.equal(form.isDirty, false);

  let notifiedStates = [];
  const unsubscribe = form.subscribe((state) => {
    notifiedStates.push(state);
  });

  form.setField("seatNumber", "D04");
  assert.equal(form.isValid, true);
  assert.equal(form.isDirty, true);
  assert.equal(form.values.seatNumber, "D04");

  unsubscribe();
  form.setField("seatNumber", "X");
  // Should not notify after unsubscribe
  assert.equal(notifiedStates.length, 2); // Initial + D04
});

test("generateTypeScriptTypes outputs clean TypeScript interfaces from schema", () => {
  const schema = defineSchema({
    userId: { type: "string", required: true },
    credits: { type: "int", required: true },
    notes: { type: "string", required: false }
  });

  const tsCode = generateTypeScriptTypes(schema, "UserQuota");
  assert.ok(tsCode.includes("export interface UserQuota {"));
  assert.ok(tsCode.includes("userId: string;"));
  assert.ok(tsCode.includes("credits: number;"));
  assert.ok(tsCode.includes("notes?: string;"));
});
