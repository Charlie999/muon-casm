
node("windows-1") {
  stage("Build CASM for Windows") {
    environment {
     DISCORD_URL = credentials("muon-discord-webhook")
    }
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      bat 'cmake .. -G "Unix Makefiles"'
      bat 'make'
      archiveArtifacts artifacts: 'casm.exe', fingerprint: true
      deleteDir()
    }
    discordSend description: "Build success", footer: "windows", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    environment {
     DISCORD_URL = credentials("muon-discord-webhook")
    }
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      sh 'cmake .. -DVERSION=latest -DBUILD_STATIC=true'
      sh 'make'
      archiveArtifacts artifacts: 'casm-static*', fingerprint: true
      deleteDir()
    }
    discordSend description: "Build success", footer: "linux", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
  }
}
