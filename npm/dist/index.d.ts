export interface HyleFieldDesc {
  name: string;
  type?: string;
  required?: boolean;
  minLength?: number;
  maxLength?: number;
  pattern?: string;
  min?: number;
  max?: number;
}

export interface ValidationError {
  field: string;
  rule: string;
  message: string;
}

export interface ValidationResult {
  valid: boolean;
  errors: ValidationError[];
}

export declare function validateRow(
  fields: HyleFieldDesc[],
  values: Record<string, any>
): ValidationError[];

export declare class HyleSchema {
  constructor(fields?: HyleFieldDesc[]);
  addField(field: HyleFieldDesc): this;
  validate(values: Record<string, any>): ValidationResult;
  toJSON(): { fields: HyleFieldDesc[] };
}

export declare function defineSchema(
  fieldMap: Record<string, Partial<HyleFieldDesc>>
): HyleSchema;

export declare function generateTypeScriptTypes(
  schemaOrFields: HyleSchema | HyleFieldDesc[],
  interfaceName?: string
): string;

export interface FormState {
  values: Record<string, any>;
  errors: ValidationError[];
  isValid: boolean;
  isDirty: boolean;
}

export interface FormValidator {
  readonly values: Record<string, any>;
  readonly errors: ValidationError[];
  readonly isValid: boolean;
  readonly isDirty: boolean;
  setField(name: string, value: any): void;
  setValues(newValues: Record<string, any>): void;
  validate(): ValidationResult;
  reset(newValues?: Record<string, any>): void;
  subscribe(callback: (state: FormState) => void): () => void;
}

export declare function createFormValidator(
  schema: HyleSchema,
  initialValues?: Record<string, any>
): FormValidator;
