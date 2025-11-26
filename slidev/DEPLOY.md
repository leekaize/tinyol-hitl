# Deploy Slides to GitHub Pages

Two options: manual or automated.

## Option 1: Manual Deploy (Quick)

```bash
cd slidev

# Build static files
npm run build

# Output in dist/ folder
# Upload dist/ contents to gh-pages branch
```

## Option 2: GitHub Actions (Recommended)

Create `.github/workflows/deploy-slides.yml`:

```yaml
name: Deploy Slides

on:
  push:
    branches: [main]
    paths:
      - 'slidev/**'
  workflow_dispatch:

jobs:
  deploy:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      pages: write
      id-token: write

    steps:
      - uses: actions/checkout@v4

      - name: Setup Node
        uses: actions/setup-node@v4
        with:
          node-version: 20

      - name: Install dependencies
        working-directory: slidev
        run: npm install

      - name: Build slides
        working-directory: slidev
        run: npm run build -- --base /tinyol-hitl/

      - name: Setup Pages
        uses: actions/configure-pages@v4

      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: slidev/dist

      - name: Deploy to GitHub Pages
        uses: actions/deploy-pages@v4
```

## Enable GitHub Pages

1. Go to repo **Settings** → **Pages**
2. Source: **GitHub Actions**
3. Push to main branch
4. Wait ~2 minutes
5. Access: `https://leekaize.github.io/tinyol-hitl/`

## Base Path Fix

The `--base /tinyol-hitl/` flag is critical. Without it, assets won't load.

If your repo name differs, change it:
```bash
npm run build -- --base /YOUR-REPO-NAME/
```

## Local Preview of Build

```bash
cd slidev
npm run build
npx serve dist
# Open http://localhost:3000
```

## Folder Structure

```
tinyol-hitl/
├── .github/
│   └── workflows/
│       └── deploy-slides.yml  # Auto-deploy
├── slidev/
│   ├── slides.md              # Your presentation
│   ├── package.json           # From npm init slidev
│   └── dist/                  # Generated (gitignored)
├── core/                      # Arduino code
└── docs/                      # Documentation
```

## Troubleshooting

**Blank page after deploy:**
- Check base path matches repo name
- Look at browser console for 404 errors

**Build fails:**
- Run `npm run build` locally first
- Check Node version (needs 18+)

**Mermaid not rendering:**
- sli.dev includes Mermaid by default
- If issues, add to `package.json`:
  ```json
  "dependencies": {
    "@slidev/cli": "latest",
    "mermaid": "^10"
  }
  ```

## Quick Commands

```bash
# Local dev
npm run dev

# Build for GitHub Pages
npm run build -- --base /tinyol-hitl/

# Export PDF (backup)
npm run export
```