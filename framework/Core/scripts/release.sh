#!/bin/bash
# PaperCrawler::Core Release Script
# Automates the release process: version bump, changelog, git tag, GitHub release

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
VERSION="$1"
RELEASE_NOTES="${RELEASE_NOTES:-RELEASE_NOTES.md}"
CHANGELOG="${CHANGELOG:-CHANGELOG.md}"
GITHUB_REPO="${GITHUB_REPO:-PaperCrawler/Core}"

# Functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

confirm() {
    local prompt="$1"
    local response

    while true; do
        read -p "${prompt} [y/N] " response
        case "$response" in
            [Yy]|[Yy][Ee][Ss])
                return 0
                ;;
            [Nn]|[Nn][Oo]|"")
                return 1
                ;;
            *)
                echo "Please answer yes or no"
                ;;
        esac
    done
}

# Validate version format
validate_version() {
    if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        print_error "Invalid version format: $VERSION"
        echo "Version must be in format: X.Y.Z (e.g., 1.0.0)"
        exit 1
    fi
}

# Check if required tools are installed
check_dependencies() {
    local missing_tools=()

    for tool in git gh; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done

    if [ ${#missing_tools[@]} -gt 0 ]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        echo "Please install them before continuing"
        exit 1
    fi
}

# Check if working directory is clean
check_git_status() {
    if [ -n "$(git status --porcelain)" ]; then
        print_warning "Working directory is not clean"
        git status

        if ! confirm "Do you want to continue anyway?"; then
            print_error "Release cancelled"
            exit 1
        fi
    fi
}

# Bump version in files
bump_version() {
    print_header "Updating Version"

    print_success "Updating version to $VERSION"

    # Update CMakeLists.txt
    if [ -f "CMakeLists.txt" ]; then
        sed -i "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $VERSION/" CMakeLists.txt
        print_success "Updated CMakeLists.txt"
    fi

    # Update package.json if exists
    if [ -f "package.json" ]; then
        sed -i "s/\"version\": \"[0-9]\+\.[0-9]\+\.[0-9]\+\"/\"version\": \"$VERSION\"/" package.json
        print_success "Updated package.json"
    fi

    # Commit version changes
    git add CMakeLists.txt package.json 2>/dev/null || true
    git commit -m "chore: bump version to $VERSION" || true
}

# Update changelog
update_changelog() {
    print_header "Updating Changelog"

    if [ ! -f "$CHANGELOG" ]; then
        print_warning "CHANGELOG.md not found, creating it"
        cat > "$CHANGELOG" <<EOF
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

EOF
    fi

    # Prepend new release section
    local temp_file=$(mktemp)
    cat > "$temp_file" <<EOF

## [$VERSION] - $(date +%Y-%m-%d)

### Added
- TODO: Add new features

### Changed
- TODO: List changes

### Deprecated
- TODO: List deprecated features

### Removed
- TODO: List removed features

### Fixed
- TODO: List bug fixes

### Security
- TODO: List security fixes

EOF

    # Append existing changelog
    cat "$CHANGELOG" >> "$temp_file"

    # Replace original
    mv "$temp_file" "$CHANGELOG"

    print_success "Updated $CHANGELOG"
    print_warning "Please edit $CHANGELOG to add release notes"

    if ! confirm "Have you finished editing the changelog?"; then
        print_error "Release cancelled"
        exit 1
    fi

    git add "$CHANGELOG"
    git commit -m "docs: update changelog for $VERSION"
}

# Create git tag
create_tag() {
    print_header "Creating Git Tag"

    # Check if tag already exists
    if git rev-parse "$VERSION" >/dev/null 2>&1; then
        print_error "Tag $VERSION already exists"
        exit 1
    fi

    # Create annotated tag
    git tag -a "$VERSION" -m "Release $VERSION"

    print_success "Created tag: $VERSION"
}

# Build packages
build_packages() {
    print_header "Building Packages"

    if [ -f "scripts/package.sh" ]; then
        bash scripts/package.sh --version "$VERSION"
        print_success "Packages built successfully"
    else
        print_warning "Package script not found, skipping"
    fi
}

# Create GitHub release
create_github_release() {
    print_header "Creating GitHub Release"

    # Generate release notes from changelog
    local release_notes_file=$(mktemp)
    sed -n "/## \[$VERSION\]/,/## \[/p" "$CHANGELOG" | head -n -1 > "$release_notes_file"

    # Create release using GitHub CLI
    gh release create "$VERSION" \
        --title "PaperCrawler::Core $VERSION" \
        --notes-file "$release_notes_file" \
        dist/*

    print_success "GitHub release created"
}

# Main flow
main() {
    # Check arguments
    if [ -z "$VERSION" ]; then
        print_error "Version not specified"
        echo "Usage: $0 VERSION"
        echo "Example: $0 1.0.0"
        exit 1
    fi

    # Validate version
    validate_version

    # Check dependencies
    check_dependencies

    # Print configuration
    print_header "PaperCrawler::Core Release"
    echo -e "Version: ${GREEN}${VERSION}${NC}"
    echo -e "Repository: ${GREEN}${GITHUB_REPO}${NC}"
    echo ""

    # Confirm release
    if ! confirm "Do you want to release version $VERSION?"; then
        print_error "Release cancelled"
        exit 1
    fi

    # Check git status
    check_git_status

    # Ensure we're on main branch
    local current_branch=$(git branch --show-current)
    if [ "$current_branch" != "main" ]; then
        print_warning "You are not on the main branch (current: $current_branch)"
        if ! confirm "Do you want to continue?"; then
            print_error "Release cancelled"
            exit 1
        fi
    fi

    # Execute release steps
    bump_version
    update_changelog
    create_tag
    build_packages
    create_github_release

    # Push to GitHub
    print_header "Pushing to GitHub"

    if confirm "Push changes and tag to GitHub?"; then
        git push
        git push --tags
        print_success "Pushed to GitHub"
    fi

    # Success!
    print_header "Release Complete!"

    echo ""
    print_success "PaperCrawler::Core $VERSION has been released successfully!"
    echo ""
    echo "Next steps:"
    echo "  1. Announce the release on social media"
    echo "  2. Update website and documentation"
    echo "  3. Notify users and stakeholders"
    echo "  4. Monitor issues and feedback"
}

main "$@"
