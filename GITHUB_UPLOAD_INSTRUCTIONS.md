# GitHub Upload Instructions

## Repository is Ready!

The git repository has been initialized and all files have been committed.

## Steps to Upload to GitHub

### 1. Create a New Repository on GitHub

1. Go to https://github.com
2. Sign in with your account (mouhamed.garrch@gmail.com)
3. Click the "+" icon in the top right → "New repository"
4. Repository name: `pqc-ledger-task` (or any name you prefer)
5. Description: "Post-Quantum Cryptography Ledger - Blockchain transaction library with Dilithium signatures"
6. Choose Public or Private
7. **DO NOT** initialize with README, .gitignore, or license (we already have these)
8. Click "Create repository"

### 2. Push to GitHub

After creating the repository, GitHub will show you commands. Use these:

```bash
cd "C:\Users\Med Amine Garrach\Downloads\technical-task-Amine"

# Add your GitHub repository as remote (replace YOUR_USERNAME with your GitHub username)
git remote add origin https://github.com/YOUR_USERNAME/pqc-ledger-task.git

# Rename branch to main (if needed)
git branch -M main

# Push to GitHub
git push -u origin main
```

### 3. Authentication

When you push, GitHub will ask for authentication. You can:

**Option A: Personal Access Token (Recommended)**
1. Go to GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Generate new token (classic)
3. Give it a name like "pqc-ledger-upload"
4. Select scopes: `repo` (full control of private repositories)
5. Copy the token
6. When prompted for password, paste the token instead

**Option B: GitHub CLI**
```bash
# Install GitHub CLI if not installed
# Then authenticate
gh auth login
```

**Option C: SSH Key**
1. Generate SSH key: `ssh-keygen -t ed25519 -C "mouhamed.garrch@gmail.com"`
2. Add to GitHub: Settings → SSH and GPG keys → New SSH key
3. Use SSH URL: `git@github.com:YOUR_USERNAME/pqc-ledger-task.git`

## Quick Command Summary

```bash
# Navigate to project
cd "C:\Users\Med Amine Garrach\Downloads\technical-task-Amine"

# Add remote (replace YOUR_USERNAME)
git remote add origin https://github.com/YOUR_USERNAME/pqc-ledger-task.git

# Push
git push -u origin main
```

## What's Included

✅ All source code
✅ All tests
✅ All documentation
✅ CMake build files
✅ README with instructions
✅ .gitignore (excludes build files and liboqs)

## Note About liboqs

The `third_party/liboqs` directory is included as a git submodule reference. Users will need to:

```bash
git submodule update --init --recursive
```

Or clone liboqs separately as documented in the README.

## After Uploading

Once uploaded, you can:
- Share the repository URL
- Clone it on other machines
- Continue development with version control
- Add collaborators

## Repository URL Format

After uploading, your repository will be at:
`https://github.com/YOUR_USERNAME/pqc-ledger-task`



