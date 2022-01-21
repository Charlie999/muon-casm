
node("windows-1") {
  stage("Build CASM for Windows") {
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      bat 'cmake .. -G "Unix Makefiles"'
      bat 'make -j4'
      archiveArtifacts artifacts: 'casm.exe', fingerprint: true
      deleteDir()
    }
    
    withCredentials([string(credentialsId: 'muon-discord-webhook', variable: 'DISCORD_URL')]) {
     discordSend description: "Build complete", footer: "windows", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    environment {
      CC  = '/usr/lib/ccache/gcc'
      CXX = '/usr/lib/ccache/g++'
    }
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      sh 'cmake .. -DVERSION=latest -DBUILD_STATIC=true'
      sh 'make -j4'
      archiveArtifacts artifacts: 'casm-static*', fingerprint: true
      deleteDir()
    }
    
    withCredentials([string(credentialsId: 'muon-discord-webhook', variable: 'DISCORD_URL')]) {
     discordSend description: "Build complete", footer: "linux", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
    }
  }
}
