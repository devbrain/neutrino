import type {ReactNode} from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import styles from './index.module.css';

// Deliberately spare. The decision on this site was "public, but substance-first": it is published
// openly, but the effort belongs in the concept pages rather than in a feature grid. A visitor
// should be able to tell within one screen whether this engine is for them, and then leave for the
// docs. If a fuller landing page is ever wanted, it goes here -- not into the docs.

function Hero() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <Heading as="h1" className="hero__title">
          {siteConfig.title}
        </Heading>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <p className={styles.heroBlurb}>
          Scenes, sprites, tile worlds, collision and audio — as a library you link against.
          No editor, no scripting layer: you write C++ and you get a binary.
        </p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/get-started/what-is-neutrino">
            Get started
          </Link>
          <Link
            className="button button--outline button--secondary button--lg"
            to="/docs/concepts/the-frame">
            Read the concepts
          </Link>
        </div>
      </div>
    </header>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="A C++20 game engine built on SDL3"
      description="Neutrino is a C++20 2D game engine built on SDL3: scenes, sprites, tile worlds, kinematic collision and audio.">
      <Hero />
    </Layout>
  );
}
