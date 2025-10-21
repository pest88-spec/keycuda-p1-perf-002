#!/bin/bash

# Puzzle71Solver - Configuration Migration Script (T057)
# Phase 7: User Story 5 - Compatibility Assurance
# Comprehensive configuration migration tool for legacy config files

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPATIBILITY_DIR="$PROJECT_ROOT/src/KeyhuntCore/compatibility"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default options
SOURCE_CONFIG=""
TARGET_CONFIG=""
TARGET_FORMAT="json_v2"
CREATE_BACKUP=true
DRY_RUN=false
FORCE_MIGRATION=false
VERBOSE=false
VALIDATE_ONLY=false
LIST_FORMATS=false
AUTO_DETECT=true

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_migration() {
    echo -e "${PURPLE}[MIGRATION]${NC} $1"
}

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS] SOURCE_CONFIG [TARGET_CONFIG]

Configuration Migration Tool for Puzzle71Solver
Migrates legacy configuration files to modern JSON format

POSITIONAL ARGUMENTS:
    SOURCE_CONFIG            Path to source configuration file (required)
    TARGET_CONFIG            Path to target configuration file (optional, auto-generated if not provided)

OPTIONS:
    --format <format>        Target format (default: json_v2)
                             Supported formats: legacy_v1, legacy_v2, json_v1, json_v2, yaml_v1, toml_v1
    --backup                 Create backup of original file (default: enabled)
    --no-backup              Don't create backup file
    --dry-run                Perform migration without writing files
    --force                  Force migration even if validation fails
    --validate-only          Only validate configuration, don't migrate
    --list-formats           List all supported formats and exit
    --no-auto-detect         Don't auto-detect source format
    --verbose                Verbose output
    --help, -h               Show this help message

FORMAT DESCRIPTIONS:
    legacy_v1    - Legacy V1 (key=value format)
    legacy_v2    - Legacy V2 (sectioned format)
    json_v1      - JSON V1 (basic JSON format)
    json_v2      - JSON V2 (enhanced JSON with metadata) [RECOMMENDED]
    yaml_v1      - YAML format (human-readable)
    toml_v1      - TOML format (Tom's Obvious, Minimal Language)

EXAMPLES:
    # Basic migration
    $0 config.txt config_new.json

    # Migrate to JSON V2 format
    $0 legacy.conf --format json_v2

    # Validate configuration only
    $0 config.conf --validate-only

    # Dry run to see what would be migrated
    $0 config.txt --dry-run --verbose

    # List supported formats
    $0 --list-formats

NOTES:
    - If TARGET_CONFIG is not specified, it will be auto-generated
    - Backups are created by default with timestamp
    - The script validates source configuration before migration
    - JSON V2 format is recommended for best compatibility

EOF
}

# Function to parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --format)
                TARGET_FORMAT="$2"
                shift 2
                ;;
            --backup)
                CREATE_BACKUP=true
                shift
                ;;
            --no-backup)
                CREATE_BACKUP=false
                shift
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --force)
                FORCE_MIGRATION=true
                shift
                ;;
            --validate-only)
                VALIDATE_ONLY=true
                shift
                ;;
            --list-formats)
                LIST_FORMATS=true
                shift
                ;;
            --no-auto-detect)
                AUTO_DETECT=false
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            -*)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
            *)
                if [[ -z "$SOURCE_CONFIG" ]]; then
                    SOURCE_CONFIG="$1"
                elif [[ -z "$TARGET_CONFIG" ]]; then
                    TARGET_CONFIG="$1"
                else
                    log_error "Too many arguments"
                    show_usage
                    exit 1
                fi
                shift
                ;;
        esac
    done
}

# Function to validate arguments
validate_arguments() {
    if [[ "$LIST_FORMATS" == "true" ]]; then
        return 0
    fi

    if [[ -z "$SOURCE_CONFIG" ]]; then
        log_error "Source configuration file is required"
        show_usage
        exit 1
    fi

    if [[ ! -f "$SOURCE_CONFIG" ]]; then
        log_error "Source configuration file not found: $SOURCE_CONFIG"
        exit 1
    fi

    # Validate target format
    case "$TARGET_FORMAT" in
        legacy_v1|legacy_v2|json_v1|json_v2|yaml_v1|toml_v1)
            # Valid format
            ;;
        *)
            log_error "Invalid target format: $TARGET_FORMAT"
            log_error "Use --list-formats to see supported formats"
            exit 1
            ;;
    esac

    # Generate target config if not provided
    if [[ -z "$TARGET_CONFIG" && "$VALIDATE_ONLY" != "true" ]]; then
        local source_ext="${SOURCE_CONFIG##*.}"
        local source_base="${SOURCE_CONFIG%.*}"

        case "$TARGET_FORMAT" in
            json_v1|json_v2)
                TARGET_CONFIG="${source_base}_migrated.json"
                ;;
            yaml_v1)
                TARGET_CONFIG="${source_base}_migrated.yaml"
                ;;
            toml_v1)
                TARGET_CONFIG="${source_base}_migrated.toml"
                ;;
            legacy_v1)
                TARGET_CONFIG="${source_base}_migrated.cfg"
                ;;
            legacy_v2)
                TARGET_CONFIG="${source_base}_migrated.conf"
                ;;
        esac

        log_info "Auto-generated target configuration: $TARGET_CONFIG"
    fi

    # Check if target already exists
    if [[ -n "$TARGET_CONFIG" && -f "$TARGET_CONFIG" && "$FORCE_MIGRATION" != "true" ]]; then
        log_error "Target configuration already exists: $TARGET_CONFIG"
        log_error "Use --force to overwrite or specify a different target file"
        exit 1
    fi
}

# Function to list supported formats
list_formats() {
    echo "Supported Configuration Formats"
    echo "==============================="
    echo
    echo "Format Code    Description                                    Extension"
    echo "-----------    -----------                                    ---------"
    echo "legacy_v1     Legacy V1 (key=value format)                    .cfg"
    echo "legacy_v2     Legacy V2 (sectioned format)                    .conf"
    echo "json_v1       JSON V1 (basic JSON format)                     .json"
    echo "json_v2       JSON V2 (enhanced JSON with metadata) [RECOMMENDED]  .json"
    echo "yaml_v1       YAML format (human-readable)                    .yaml"
    echo "toml_v1       TOML format (Tom's Obvious, Minimal Language)  .toml"
    echo
    echo "Migration Recommendations:"
    echo "- Use json_v2 for best compatibility and features"
    echo "- Use yaml_v1 for human-readable configurations"
    echo "- Use toml_v1 for minimal, clean configurations"
    echo
    echo "Legacy formats (legacy_v1, legacy_v2) are supported for migration but not recommended for new configurations."
}

# Function to detect configuration format
detect_config_format() {
    local config_file="$1"
    log_step "Detecting configuration format..."

    if [[ "$AUTO_DETECT" != "true" ]]; then
        log_info "Auto-detection disabled, assuming unknown format"
        echo "unknown"
        return
    fi

    # Read first few lines to determine format
    local sample=$(head -20 "$config_file")

    # Check for JSON format
    if echo "$sample" | grep -q "^\s*{"; then
        # Check if it's JSON V2 (has metadata/version)
        if echo "$sample" | grep -q "\"version\"\|\"metadata\""; then
            echo "json_v2"
            log_success "Detected format: JSON V2"
        else
            echo "json_v1"
            log_success "Detected format: JSON V1"
        fi
        return
    fi

    # Check for YAML format
    if echo "$sample" | grep -q "^[a-zA-Z_][a-zA-Z0-9_]*\s*:"; then
        echo "yaml_v1"
        log_success "Detected format: YAML V1"
        return
    fi

    # Check for TOML format
    if echo "$sample" | grep -q "^\[.*\]\|^[a-zA-Z_][a-zA-Z0-9_]*\s*="; then
        echo "toml_v1"
        log_success "Detected format: TOML V1"
        return
    fi

    # Check for legacy formats
    if echo "$sample" | grep -q "^\[.*\]"; then
        echo "legacy_v2"
        log_success "Detected format: Legacy V2 (sectioned)"
        return
    fi

    if echo "$sample" | grep -q "^[a-zA-Z_][a-zA-Z0-9_]*\s*="; then
        echo "legacy_v1"
        log_success "Detected format: Legacy V1 (key=value)"
        return
    fi

    echo "unknown"
    log_warning "Could not detect configuration format"
}

# Function to validate configuration
validate_config() {
    local config_file="$1"
    local format="$2"
    log_step "Validating configuration..."

    # Basic file validation
    if [[ ! -f "$config_file" ]]; then
        log_error "Configuration file not found: $config_file"
        return 1
    fi

    if [[ ! -r "$config_file" ]]; then
        log_error "Configuration file is not readable: $config_file"
        return 1
    fi

    if [[ ! -s "$config_file" ]]; then
        log_error "Configuration file is empty: $config_file"
        return 1
    fi

    # Format-specific validation
    case "$format" in
        json_v1|json_v2)
            if ! python3 -c "import json; json.load(open('$config_file'))" 2>/dev/null; then
                log_error "Invalid JSON syntax in configuration file"
                return 1
            fi
            ;;
        yaml_v1)
            if command -v python3 >/dev/null 2>&1; then
                if ! python3 -c "import yaml; yaml.safe_load(open('$config_file'))" 2>/dev/null; then
                    log_error "Invalid YAML syntax in configuration file"
                    return 1
                fi
            else
                log_warning "Python3 not available for YAML validation"
            fi
            ;;
        toml_v1)
            if command -v python3 >/dev/null 2>&1; then
                if ! python3 -c "import tomllib; tomllib.load(open('$config_file', 'rb'))" 2>/dev/null; then
                    log_error "Invalid TOML syntax in configuration file"
                    return 1
                fi
            else
                log_warning "Python3 not available for TOML validation"
            fi
            ;;
        legacy_v1|legacy_v2)
            # Basic validation for legacy formats
            local invalid_lines=$(grep -v "^[[:space:]]*#" "$config_file" | grep -v "^[[:space:]]*$" | grep -v "^[[:space:]]*\[.*\][[:space:]]*$" | grep -v "^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*=" || true)
            if [[ -n "$invalid_lines" && "$FORCE_MIGRATION" != "true" ]]; then
                log_warning "Found potentially invalid lines in legacy configuration"
                if [[ "$VERBOSE" == "true" ]]; then
                    echo "$invalid_lines" | head -5 | while read -r line; do
                        log_warning "  $line"
                    done
                fi
            fi
            ;;
    esac

    log_success "Configuration validation passed"
    return 0
}

# Function to create backup
create_backup() {
    local source_file="$1"

    if [[ "$CREATE_BACKUP" != "true" ]]; then
        return 0
    fi

    log_step "Creating backup..."

    local timestamp=$(date +"%Y%m%d_%H%M%S")
    local source_dir=$(dirname "$source_file")
    local source_name=$(basename "$source_file")
    local source_base="${source_name%.*}"
    local source_ext="${source_name##*.}"

    local backup_file="${source_dir}/${source_base}_backup_${timestamp}.${source_ext}"

    if cp "$source_file" "$backup_file"; then
        log_success "Backup created: $backup_file"
        BACKUP_FILE="$backup_file"
        return 0
    else
        log_error "Failed to create backup file"
        return 1
    fi
}

# Function to migrate configuration
migrate_config() {
    local source_file="$1"
    local target_file="$2"
    local source_format="$3"
    local target_format="$4"

    log_step "Migrating configuration from $source_format to $target_format..."

    # For dry run, just show what would be done
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "DRY RUN: Would migrate $source_file to $target_file"
        log_info "DRY RUN: Source format: $source_format"
        log_info "DRY RUN: Target format: $target_format"
        return 0
    fi

    # Perform actual migration based on formats
    case "$source_format" in
        legacy_v1|legacy_v2)
            migrate_legacy_to_modern "$source_file" "$target_file" "$target_format"
            ;;
        json_v1)
            migrate_json_v1_to_v2 "$source_file" "$target_file"
            ;;
        yaml_v1|toml_v1)
            migrate_yaml_toml_to_json "$source_file" "$target_file" "$target_format"
            ;;
        json_v2)
            if [[ "$target_format" != "json_v2" ]]; then
                migrate_json_v2_to_other "$source_file" "$target_file" "$target_format"
            else
                log_info "Source and target formats are the same, copying file"
                cp "$source_file" "$target_file"
            fi
            ;;
        *)
            log_error "Unsupported source format: $source_format"
            return 1
            ;;
    esac

    if [[ $? -eq 0 ]]; then
        log_success "Configuration migrated successfully"
        return 0
    else
        log_error "Migration failed"
        return 1
    fi
}

# Function to migrate legacy to modern format
migrate_legacy_to_modern() {
    local source_file="$1"
    local target_file="$2"
    local target_format="$3"

    log_info "Migrating legacy configuration to $target_format..."

    # Create temporary working file
    local temp_file=$(mktemp)

    # Process legacy configuration
    {
        echo "{"
        echo "  \"metadata\": {"
        echo "    \"version\": \"2.0\","
        echo "    \"format\": \"$target_format\","
        echo "    \"generated_at\": \"$(date -Iseconds)\","
        echo "    \"generator\": \"Puzzle71Solver Config Migrator\","
        echo "    \"source_format\": \"legacy\","
        echo "    \"source_file\": \"$(basename "$source_file")\""
        echo "  },"
        echo "  \"sections\": {"

        local current_section=""
        local first_section=true

        while IFS= read -r line; do
            # Skip comments and empty lines
            [[ "$line" =~ ^[[:space:]]*# ]] && continue
            [[ "$line" =~ ^[[:space:]]*$ ]] && continue

            # Check for section header
            if [[ "$line" =~ ^[[:space:]]*\[(.+)\][[:space:]]*$ ]]; then
                # Close previous section if not first
                if [[ "$first_section" != "true" ]]; then
                    echo "      }"
                fi

                current_section="${BASH_REMATCH[1]}"
                [[ "$first_section" != "true" ]] && echo ","
                echo "    \"$current_section\": {"
                echo "      \"parameters\": {"
                first_section=false
                continue
            fi

            # Parse key=value pair
            if [[ "$line" =~ ^[[:space:]]*([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]*=[[:space:]]*(.+)[[:space:]]*$ ]]; then
                local key="${BASH_REMATCH[1]}"
                local value="${BASH_REMATCH[2]}"

                # Remove quotes from value if present
                value="${value//\"/}"
                value="${value//\'/}"

                # Convert to appropriate JSON type
                if [[ "$value" =~ ^(true|false)$ ]]; then
                    # Boolean value
                    [[ "$first_section" != "true" ]] && echo ","
                    echo "        \"$key\": $value"
                elif [[ "$value" =~ ^[0-9]+$ ]]; then
                    # Integer value
                    [[ "$first_section" != "true" ]] && echo ","
                    echo "        \"$key\": $value"
                elif [[ "$value" =~ ^[0-9]+\.[0-9]+$ ]]; then
                    # Float value
                    [[ "$first_section" != "true" ]] && echo ","
                    echo "        \"$key\": $value"
                else
                    # String value
                    [[ "$first_section" != "true" ]] && echo ","
                    echo "        \"$key\": \"$value\""
                fi

                first_section=false
            fi
        done < "$source_file"

        # Close last section if we had sections
        if [[ -n "$current_section" ]]; then
            echo "      }"
            echo "    }"
        else
            # No sections found, create default section
            echo "    \"default\": {"
            echo "      \"parameters\": {}"
            echo "    }"
        fi

        echo "  }"
        echo "}"
    } > "$temp_file"

    # Convert to target format if needed
    case "$target_format" in
        json_v1|json_v2)
            cp "$temp_file" "$target_file"
            ;;
        yaml_v1)
            if command -v python3 >/dev/null 2>&1; then
                python3 -c "import json, yaml; yaml.dump(json.load(open('$temp_file')), open('$target_file', 'w'), default_flow_style=False)"
            else
                log_error "Python3 required for YAML output"
                rm -f "$temp_file"
                return 1
            fi
            ;;
        toml_v1)
            if command -v python3 >/dev/null 2>&1; then
                python3 -c "import json, tomllib; json_data = json.load(open('$temp_file')); toml_data = {}; import toml; toml.dump(toml_data, open('$target_file', 'w'))"
            else
                log_error "Python3 and tomllib required for TOML output"
                rm -f "$temp_file"
                return 1
            fi
            ;;
    esac

    rm -f "$temp_file"
}

# Function to migrate JSON V1 to V2
migrate_json_v1_to_v2() {
    local source_file="$1"
    local target_file="$2"

    log_info "Migrating JSON V1 to V2 format..."

    # Use Python to migrate JSON V1 to V2
    if command -v python3 >/dev/null 2>&1; then
        python3 << EOF
import json
import sys
from datetime import datetime

# Load source JSON
with open('$source_file', 'r') as f:
    data = json.load(f)

# Create V2 structure
v2_data = {
    "metadata": {
        "version": "2.0",
        "format": "json_v2",
        "generated_at": datetime.now().isoformat(),
        "generator": "Puzzle71Solver Config Migrator",
        "source_format": "json_v1",
        "source_file": "$(basename "$source_file")"
    },
    "sections": {}
}

# Migrate data to sections format
if isinstance(data, dict):
    # Check if it's already in sections format
    if "sections" in data:
        v2_data["sections"] = data["sections"]
    else:
        # Wrap everything in a default section
        v2_data["sections"]["default"] = {
            "parameters": data
        }

# Write target JSON
with open('$target_file', 'w') as f:
    json.dump(v2_data, f, indent=2)

print("JSON V1 to V2 migration completed successfully")
EOF
    else
        log_error "Python3 required for JSON V1 to V2 migration"
        return 1
    fi
}

# Function to migrate YAML/TOML to JSON
migrate_yaml_toml_to_json() {
    local source_file="$1"
    local target_file="$2"
    local target_format="$3"

    log_info "Migrating $(basename "$source_file") to JSON V2 format..."

    if command -v python3 >/dev/null 2>&1; then
        python3 << EOF
import json
import sys
from datetime import datetime
import os

source_file = '$source_file'
target_file = '$target_file'
target_format = '$target_format'

# Determine source format from file extension
_, ext = os.path.splitext(source_file)
ext = ext.lower()

data = None

try:
    if ext in ['.yaml', '.yml']:
        import yaml
        with open(source_file, 'r') as f:
            data = yaml.safe_load(f)
    elif ext == '.toml':
        try:
            import tomllib
            with open(source_file, 'rb') as f:
                data = tomllib.load(f)
        except ImportError:
            import toml
            with open(source_file, 'r') as f:
                data = toml.load(f)
    else:
        print("Error: Unsupported source format")
        sys.exit(1)
except Exception as e:
    print(f"Error parsing source file: {e}")
    sys.exit(1)

if data is None:
    print("Error: No data loaded from source file")
    sys.exit(1)

# Create V2 structure
v2_data = {
    "metadata": {
        "version": "2.0",
        "format": "json_v2",
        "generated_at": datetime.now().isoformat(),
        "generator": "Puzzle71Solver Config Migrator",
        "source_format": ext[1:],  # Remove the dot
        "source_file": os.path.basename(source_file)
    },
    "sections": {}
}

# Migrate data to sections format
if isinstance(data, dict):
    if "sections" in data:
        v2_data["sections"] = data["sections"]
    else:
        # Use top-level keys as sections or put everything in default
        has_nested_sections = any(isinstance(v, dict) for v in data.values() if v is not None)

        if has_nested_sections:
            # Use top-level keys as sections
            for key, value in data.items():
                if key != 'metadata' and isinstance(value, dict):
                    v2_data["sections"][key] = {
                        "parameters": value
                    }
                elif key != 'metadata':
                    # Non-dict value goes in default section
                    if "default" not in v2_data["sections"]:
                        v2_data["sections"]["default"] = {"parameters": {}}
                    v2_data["sections"]["default"]["parameters"][key] = value
        else:
            # Put everything in default section
            v2_data["sections"]["default"] = {
                "parameters": data
            }

# Write output in target format
if target_format in ['json_v1', 'json_v2']:
    with open(target_file, 'w') as f:
        json.dump(v2_data, f, indent=2)
elif target_format == 'yaml_v1':
    import yaml
    with open(target_file, 'w') as f:
        yaml.dump(v2_data, f, default_flow_style=False)
elif target_format == 'toml_v1':
    import toml
    with open(target_file, 'w') as f:
        toml.dump(v2_data, f)

print("Migration completed successfully")
EOF
    else
        log_error "Python3 required for YAML/TOML to JSON migration"
        return 1
    fi
}

# Function to validate migrated configuration
validate_migrated_config() {
    local config_file="$1"
    local format="$2"

    if [[ "$VALIDATE_ONLY" == "true" ]]; then
        return 0
    fi

    log_step "Validating migrated configuration..."

    validate_config "$config_file" "$format"
}

# Function to show migration summary
show_migration_summary() {
    log_step "Migration Summary"
    echo
    echo "Source Configuration: $SOURCE_CONFIG"
    echo "Target Configuration: ${TARGET_CONFIG:-"N/A (validate only)"}"
    echo "Source Format: $DETECTED_FORMAT"
    echo "Target Format: $TARGET_FORMAT"
    echo "Backup Created: ${BACKUP_FILE:-"No"}"
    echo "Dry Run: $DRY_RUN"
    echo
    echo "Status: ${MIGRATION_SUCCESS:-"Pending"}"
    echo

    if [[ "$MIGRATION_SUCCESS" == "true" ]]; then
        log_success "✅ Migration completed successfully!"

        if [[ "$VALIDATE_ONLY" != "true" ]]; then
            echo "Next steps:"
            echo "1. Review the migrated configuration: $TARGET_CONFIG"
            echo "2. Test the new configuration with your application"
            echo "3. Update your scripts/documentation to use the new file"

            if [[ "$CREATE_BACKUP" == "true" && -n "$BACKUP_FILE" ]]; then
                echo "4. Keep the backup file until you've verified everything works: $BACKUP_FILE"
            fi
        fi
    else
        log_error "❌ Migration failed!"
        echo "Please check the error messages above and fix any issues."
    fi
}

# Function to cleanup on exit
cleanup() {
    # Remove any temporary files
    rm -f /tmp/config_migrate_*.tmp 2>/dev/null || true
}

# Main execution function
main() {
    echo "=============================================="
    echo "Configuration Migration Tool"
    echo "Puzzle71Solver CUDA Technical Debt Elimination"
    echo "=============================================="
    echo

    # Parse arguments
    parse_arguments "$@"

    # Handle list formats
    if [[ "$LIST_FORMATS" == "true" ]]; then
        list_formats
        exit 0
    fi

    # Validate arguments
    validate_arguments

    # Set up cleanup
    trap cleanup EXIT

    # Detect source format
    DETECTED_FORMAT=$(detect_config_format "$SOURCE_CONFIG")

    # Validate source configuration
    if ! validate_config "$SOURCE_CONFIG" "$DETECTED_FORMAT"; then
        if [[ "$FORCE_MIGRATION" != "true" ]]; then
            log_error "Source configuration validation failed"
            log_error "Use --force to proceed with migration anyway"
            exit 1
        else
            log_warning "Proceeding with migration despite validation failures (force mode)"
        fi
    fi

    # If validate only, exit here
    if [[ "$VALIDATE_ONLY" == "true" ]]; then
        log_success "Configuration validation completed"
        exit 0
    fi

    # Create backup
    if ! create_backup "$SOURCE_CONFIG"; then
        if [[ "$FORCE_MIGRATION" != "true" ]]; then
            log_error "Failed to create backup file"
            log_error "Use --force to proceed without backup or --no-backup to disable backup creation"
            exit 1
        else
            log_warning "Proceeding without backup (force mode)"
        fi
    fi

    # Perform migration
    if migrate_config "$SOURCE_CONFIG" "$TARGET_CONFIG" "$DETECTED_FORMAT" "$TARGET_FORMAT"; then
        MIGRATION_SUCCESS=true

        # Validate migrated configuration
        if validate_migrated_config "$TARGET_CONFIG" "$TARGET_FORMAT"; then
            log_success "Migrated configuration validation passed"
        else
            log_warning "Migrated configuration validation failed - please review manually"
        fi
    else
        MIGRATION_SUCCESS=false
        exit 1
    fi

    # Show summary
    show_migration_summary

    exit 0
}

# Execute main function
main "$@"