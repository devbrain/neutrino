import {themes as prismThemes} from 'prism-react-renderer';
import type {Config} from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

// The published location. These two are the ONLY places the deployment URL is spelled out:
// every doc-to-doc link is relative and every asset goes through useBaseUrl()/require(), so
// moving to a custom domain later is a change to these two lines plus a static/CNAME file.
// Until that decision is made, the GitHub Pages project URL is the default.
const url = 'https://neutrino.igor-gutnik.workers.dev';
const baseUrl = '/';

const config: Config = {
  title: 'Neutrino',
  tagline: 'A C++20 game engine built on SDL3',
  favicon: 'img/favicon.ico',

  future: {
    v4: true, // Improve compatibility with the upcoming Docusaurus v4
  },

  url,
  baseUrl,

  organizationName: 'devbrain',
  projectName: 'neutrino',

  // Broken links are a build failure, not a warning. The docs cross-link heavily between the
  // tutorial and the concept pages, and a silently dead link is worse than a red build.
  onBrokenLinks: 'throw',
  // Anchors too: these pages deep-link into each other's sections, and a heading rename would
  // otherwise silently degrade those links to "lands on the page, wrong place".
  onBrokenAnchors: 'throw',
  markdown: {
    hooks: {
      // v4 location for what used to be the top-level onBrokenMarkdownLinks.
      onBrokenMarkdownLinks: 'throw',
    },
  },

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.ts',
          editUrl: 'https://github.com/devbrain/neutrino/tree/main/website/',
          showLastUpdateTime: true,
        },
        // No blog: this is reference material, not a news feed. Re-enable if release notes
        // ever want a chronological home.
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    colorMode: {
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: 'Neutrino',
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'docsSidebar',
          position: 'left',
          label: 'Docs',
        },
        {
          // Doxygen output, published beside the site rather than inside it. Not a relative
          // doc link -- it is a separate generated artefact, so it is pathname:// to stop
          // Docusaurus trying to resolve it at build time.
          href: 'pathname:///api/',
          label: 'API',
          position: 'left',
        },
        {
          href: 'https://github.com/devbrain/neutrino',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Docs',
          items: [
            {label: 'Get started', to: '/docs/get-started/what-is-neutrino'},
            {label: 'Concepts', to: '/docs/concepts/the-frame'},
          ],
        },
        {
          title: 'Reference',
          items: [
            {label: 'API (Doxygen)', href: 'pathname:///neutrino/api/'},
          ],
        },
        {
          title: 'More',
          items: [
            {label: 'GitHub', href: 'https://github.com/devbrain/neutrino'},
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} Neutrino. Built with Docusaurus.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
      // The scaffold ships neither; nearly every snippet on this site is one or the other.
      additionalLanguages: ['cpp', 'cmake', 'bash'],
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
